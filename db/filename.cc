// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include <ctype.h>
#include <stdio.h>
#include "db/filename.h"
#include "db/dbformat.h"
#include "leveldb/env.h"
#include "util/logging.h"

namespace leveldb {

static std::string MakeFileName(const std::string& name, uint64_t number,
                                const char* suffix) {
    char buf[100];
    snprintf(buf, sizeof(buf), "/%06llu.%s",
            static_cast<unsigned long long>(number),
            suffix);
    return name + buf;
}

std::string LogFileName(const std::string& name, uint64_t number) {
    assert(number > 0);
    return MakeFileName(name, number, "log");
}


}   // namespace leveldb

