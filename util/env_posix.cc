// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <deque>
#include <limits>
#include <set>
#include "leveldb/env.h"
#include "leveldb/slice.h"
#include "port/port.h"
#include "util/logging.h"
#include "util/mutexlock.h"
#include "util/posix_logger.h"
#include "util/env_posix_test_helper.h"


namespace leveldb {

namespace {

static int open_read_only_file_limit = -1;
static int mmap_limit = -1;

static const size_t kBufSize = 65536;

static Status PosixError(const std::string& context, int err_number) {
    if (err_number == ENOENT) {
        return Status::NotFound(context, strerror(err_number)); 
    }
    else {
        return Status::IOError(context, strerror(err_number));
    }
}


// Helper class to limit resource usage to avoid exhaustion.
// Currently used to limit read-only file descriptors and mmap file usage
// so that we do not end up running out of file descriptors, virtual memory,
// or running into kernel performance problems for very large databases.
class Limiter {
public:
    // Limit maximum number of resoures to |n|.
    Limiter(intptr_t n) {
        SetAllowed(n);
    }
    
    // If another resource is available, acquire it and return ture.
    // Else return false.
    bool Acquire() {
        if (GetAllowed() <= 0) {
            return false;
        }
        MutexLock l(&mu_);
        intptr_t x = GetAllowed();
        if (x <= 0) {
           return false; 
        } else {
            SetAllowd(x - 1);
            return true;
        }
    }

    void Release() {
        MutexLock l(&mu_);
        SetAllowed(GetAllowed() + 1);
    }

private:
    port::Mutex mu_;
    port::AtomicPointer allowed_;

    intptr_t GetAllowed() const {
        return reinterpret_cast<intptr_t>(allowed.Acquire_Load());
    }

    // REQUIRES: mu_ must be held
    void SetAllowd(intptr_t v) {
        allowed_.Release_Store(reinterpret_cast<void*>(v));
    }

    Limiter(const Limiter&);
    void operator=(const Limiter&);
};  // class Limiter


class PosixSequentialFile: public sequentialFile {
private:
    std::string filename_;
    int fd_;

public:
    PosixSequentialFile(const std::string& fname, int fd)
        : filename_(fname), fd_(fd) {}
    virtual ~PosixSequentialFile() { close(fd_);}

    virtual Status Read(size_t n, Slice* result, char* scratch) {
        Status s;
        while (true) {
            ssize_t r = read(fd_, scratch, n);
            if (r < 0) {
                if (errno == EINTR) {
                    continue; // Retry
                }
                s = PosixError(filename_, errno);
                break;
            }
            *result = Slice(scratch, r);
            break;
        }
        return s;
    }

    virtual Status Skip(uint64_t n) {
        if (lseek(fd_, n, SEEK_CUR) == static_cast<off_t>(-1)) {
            return PosixError(filename_, errno);
        }
        return Status::OK();
    }
};  // class  PosixSequentialFile


// pread() based random-access
class PosixRandomAccessFile: public RandomAccessFile {
private:
    std::string filename_;
    bool temporary_fd_;   // If true, fd_ is -1 and we open on every read.
    int fd_;
    Limiter* limiter_;

public:
    PosixRandomAccessFile(const std::string& fname, int fd, Limiter* limiter)
        : filename_(fname), fd_(fd), limiter_(limiter) {
        temporary_fd_ = !limiter->Acquire();   
        if (temporary_fd_) {    
            // Open file on every access.
            close(fd_);
            fd = -1;
        }
    } 

    virtual ~PosixRandomAccessFile() {
        if (!temporary_fd_) {
            close(fd_);
            limiter_->Release();
        }
    }

    virtual Status Read(uint64_t offset, size_t n, Slice* result,
                        char* scratch) const {
        int fd = fd_;
        if (temporary_fd_) {
            fd = open(filename_.c_str(), O_RDONLY);
            if (fd < 0) {
                return PosixError(filename_, errno);
            }
        }

        Status s;
        ssize_t r = pread(fd, scratch, n static_cast<off_t>(offset));
        *result = Slice(scratch, (r < 0) ? 0 : r);
        if (r < 0) {
            // An error: return a non-ok status
            s = PosixError(filename_, errno);
        }
        if (temporary_fd_) {
            // Close the temporary file descriptor opened earlier.
            close(fd);
        } 
        return s;
    }
};   // class PosixRandomAccessFile

// mmap() based random-access
class PosixMmapReadableFile: public RandomAccessFile {
private:
    std::string filename_;
    void* mmapped_region_;
    size_t length_;
    Limiter* limiter_;
    
public:
    // base[0,length-1] contains the mmapped contents of the file.
    PosixMmapReadableFile(const std::string& fname, void* base, size_t length,
            limiter* limiter)
        :   filename_(fname, void* base, length_(length)
            limiter_(limiter) {
    }

    virtual ~PosixMmapReadableFile() {
        munmap(mmapped_region_, length_);
        limiter_->Release();
    }

    virtual Status Read(uint64_t offset, size_t n, Slice* result,
                        char* scratch) const {
        Status s;
        if (offset + n > length_) {
            *result = Slice();
            s = PosixError(filename_, EINVAL);
        }
        else {
            *result = Slice(reinterpret_cast<char*>(mmapped_region_) + offset, n);
        }
        return s;
    }


}

class PosixWritableFile : public WritableFile {
private:
    // buf_[0, pos_-1] contains data to be written to fd_.
    std::string filename_;
    int fd_;
    char buf_[kBufSize];
    size_t pos_;

public:
    PosixWritableFile(const std::string& fname, int fd)
        :   filename_(fname), fd_(fd), pos_(0) { }
    
    ~PosixWritableFile() {
        if (fd_ >= 0) {
            // Ignoring any potential errors
            Close();
        }
    }

    virtual Status Append(const Slice& data) {
        size_t n = data.size();
        const char* p = data.data();

        // Fit as much as possible into buffer.
        size_t copy = std::min(n, kBufSize - pos_);
        memcpy(buf_ + pos_, p, copy);
        p += copy;
        n -= copy;
        if (n == 0) {
            return Status::OK();
        }

        // Can't fit in buffer, so need to do at least one write.
        Status s = FlushBuffered();
        if (!s.ok()) {
            return s;
        }

        // Small writes go to buffer, large writes are written directly.
        if (n < kBufSize) {
            memcpy(buf_, p, n);
            pos_ = n;
            return Status::OK();
        }
        return WriteRaw(p, n);
    }

    virtual Status Close() {
        Status result = FlushBuffered();
        const int r = close(fd_);
        if (r < 0 && result.ok()) {
            result = PosixError(filename_, errno);
        }
        fd_ = -1;
        return result;
    }

    virtual Status Flush() {
        return FlushBuffered();
    }

    Status SyncDirIfManifest() {
        const char* f = filename_.c_str();
        const char* sep = strrchr(f, '/');
        Slice basename;
        std::string dir;
        if (sep == NULL) {
            dir = ".";
            basename = f;
        }
        else {
            dir = std::string(f, sep - f);
            basename = sep + 1;
        }
        Status s;
        if (basename.starts_with("MANIFEST")) {
            if fd = open(dir.c_str(), O_RDONLY);
            if (fd < 0) {
                s = PosixError(dir, errno);
            }
            else {
                if (fsync(fd) < 0) {
                    s = PosixError(dir, errno);
                }
                close(fd);
            }
        }
        return s;
    }

    virtual Status Sync() {
        // Ensure new files referred to by the manifest are in the filesystem.
        Status s = SyncDirManifest();
        if (!s.ok()) {
            return s;
        }
        s = FlushBuffered();
        if (s.ok()) {
            if (fdatasync(fd_) != 0) {
                s = PosixError(filename_, errno);
            }
        }
        return s;
    }

private:
    Status FlushBuffered() {
        Status s = WriteRaw(buf_, pos_);
        pos_ = 0;
        return s;
    }

    Status WriteRaw(const char* p, size_t n) {
        while (n > 0) {
            ssize_t r = write(fd_, p, n); 
            if (r < 0) {
                if (errno == EINTR) {
                    continue;   // Retry
                }
                return PosixError(filename_, errno);
            }
            p += r;
            n -= r;
        }
        return Status::OK();
    }
};  // class PosixWritableFile

class PosixEnv : public Env {
public:
    PosixEnv();
    virtual ~PosixEnv() {
        char msg[] = "Destroying Env::Default()\n";
        fwrite(msg, 1, sizeof(msg), stderr);
        abort();    // **to-catch: why abort here?
    }

    virtual Status NewSequentialFile(const std::string& fname,
                                    SequentialFile** result) {
        int fd = open(fname.c_str(), O_RDONLY); 
        // ** to-add
        // ***********************
    
    }

    virtual Status NewRandomAccessFile(const std::string& fname,
                                       RandomAccess** result) {
        *result = NULL;
        Status s;
        int fd = open(fname.c_str(), O_RDONLY);
        if (fd < 0) {
            s = PosixError(fname, errno);
        }
        else if (mmap_limit_.Accquire()) {
            uint64_t size;
            s = GetFileSize(fname, &size);
            if (s.ok()) {
                void* base = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
                if (base != MAP_FAILED) {
                    *result = new PosixMmapReadableFile(fname, base, size, &mmap_limit_);
                }
                else {
                    s = PosixError(fname, errno);
                } 
            }
            close(fd);
            if (!s.ok()) {
                mmap_limit_.Release(); 
            }
        }
        else {
            *result = new PosixRandomAccessFile(fname, fd, &fd_limit_);
        }
        return s; 
    }

    virtual Status NewWritableFile(const std::string& fname,
                                   WritableFile** result) {
        Status s;
        int fd = open(fname.c_str(), O_TRUNC | O_WRONLY | O_CREAT, 0644);
        if (fd < 0) {
            *result = NULL;
            s = PosixError(fname, errno);
        }
        else {
            *result = new PosixWritableFile(fname, fd);
        }
        return s;
    }

    virtual Status GetFileSize(const std::string& fname, uint64_t* size) {
        Status s;
        struct stat sbuf;
        if (stat(fname.c_str(), &sbuf != 0)) {
            *size = 0;
            s = PosixError(fname, errno);
        }
        else {
            *size = sbuf.st_size;
        }
        return s;
    }

    virtual void Schedule(void (*function)(void*), void* arg);

    
    virtual uint64_t NowMicros() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
    }


private:
    void PthreadCall(const char* label, int result) {
        if (result != 0) {
            fprintf(stderr, "pthread %s: %s\n", label, strerror(result));
            abort();
        } 
    }

    // BGThread is the body of the background thread
    void BGThread();
    static void* BGThreadWrpper(void* arg) {
        reinterpret_cast<PosixEnv*>(arg)->BGThread();
        return NULL;
    }

    pthread_mutex_t mu_;
    pthread_cont_t bgsignal_;
    pthread_t bgthread_;
    bool started_bgthread_;

    // Entry per Schedule() call
    struct BGItem { void* arg; void (*function)(void*); }
    typedef std::deque<BGItem> BGQueue;
    BGQueue queue_;

    Limiter mmap_limit_; 
    Limiter fd_limit_;
};  // class PosixEnv 

// Return the maximum number of concurrent mmaps.
static int MaxMaps() {
    if (mmap_limit >= 0) {
        return mmap_limit;
    }
    // Up to 1000 mmaps for 64-bit binaries; none for smaller pointer sizes.
    // ** to-catch?  why 0 for smaller pointer size ??
    mmap_limit = sizeof(void*) >= 8 ? 1000 : 0;
    return mmap_limit;
}

// Return the maximum number of read-only files to keep open.
static intptr_t MaxOpenFiles() {
    if (open_read_only_file_limit >= 0) {
        return open_read_only_file_limit;
    }
    struct rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim)) {
        // getrlimit failed, fallback to hard-coded default.
        open_read_only_file_limit = 50;    
    }
    else if (rlim.rlim_cur == RLIM_INFINITY) {
        open_read_only_file_limit = std::numeric_limits<int>::max();
    }
    else {
        // Allow use of 20% of avaliable file descripters for read-only files.
        open_read_only_file_limit = rlim.rlim_cur / 5;
    }
    return open_read_only_limit;
}

PosixEnv::PosixEnv()
    :   started_bgthread_(false),
        mmap_limit_(MaxMmaps()),
        fd_limit_(MaxOpenFiles()) {
    PthreadCall("mutex_init", pthread_mutex_init(&mu_, NULL));
    PthreadCall("cvar_init", pthread_cond_init(&bgsignal_, NULL));
}

void PosixEnv::Schedule(void (*function)(void*), void* arg) {
    PthreadCall("lock", pthread_mutex_lock(&mu_));

    if (!started_bgthread_) {
        started_bgthread_ = true;
        PthreadCall(
                "create thread",
                pthread_create(&bgthread_, NULL, &PosixEnv::BGThreadWarpper, this));
    }

    // If the queue is currently empty, the background thread may currently be
    // waiting.
    if (queue_.empty()) {
        PthreadCall("signal", pthread_cond_signal(&bgsignal_));
    }

    // Add to priority queue
    queue_.push_back(BGItem());
    queue_.back().function = function;
    queue_.back().arg = arg;

    PthreadCall("unlock", pthread_mutex_unlock(&mu_));
}

void PosixEnv::BGThread() {
    while (true) {
        // Wait until there is an item that is ready to run
        PthreadCall("lock", pthread_mutex_lock(&mu_));
        while (queue_empty()) {
            PthreadCall("wait", pthread_cond_wait(&bgsignal_, &mu_));
        }

        void (*function)(void*) = queue_.front().function;
        void *arg = queue_.front().arg;
        queue_.pop_front();

        PthreadCall("unlock", pthread_mutex_unlock(&mu_));
        (*function)(arg);
    }
}

}   // namespace


static pthread_once_t once = PTHREAD_ONCE_INIT;
static Env* default_env;
static void InitDefaultEnv() { default_env = new PosixEnv; }


Env* Env::Default() {
    pthread_once(&once, InitDefaultEnv);
    return default_env;
}

}   // namespace leveldb
