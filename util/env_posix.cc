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
    bool temporary_fd_;     // If true, fd_ is -1 and we open on every read.
    int fd_;
    Limiter* limiter_;

public:
    PosixRandomAccessFile(const std::string& fname, int fd, Limiter* limiter)
        : filename_(fname), fd_(fd), limiter_(limiter) {
        temporary_fd_ = !limiter->Acquire();    // ** to-catch: what is done here??
        if (temporary_fd_) {
            // Open file on every access.
            close(fd_);
            fd = -1;
        }
    } 

    virtual ~PosixRandomAccessFile() {
        if (!temporary_fd_) {
            
        }
    }
}


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
    
    }
    

};  // class PosixEnv 


static pthread_once_t once = PTHREAD_ONCE_INIT;
static Env* default_env;
static void InitDefaultEnv() { default_env = new PosixEnv; }


Env* Env::Default() {
    pthread_once(&once, InitDefaultEnv);
    return default_env;
}

}   // namespace leveldb
