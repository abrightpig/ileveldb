// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "leveldb/env.h"

namespace leveldb {

Env::~Env() {
}


void Log(Logger* info_log, const char* format, ...) {
    if (info_log != NULL) {
        va_list ap;
        va_start(ap, format);
        info_log->Logv(format, ap);
        va_end(ap);
    }
}

}   // namespace leveldb
