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
    

} 


static pthread_once_t once = PTHREAD_ONCE_INIT;
static Env* default_env;
static void InitDefaultEnv() { default_env = new PosixEnv; }


Env* Env::Default() {
    pthread_once(&once, InitDefaultEnv);
    return default_env;
}

}   // namespace leveldb
