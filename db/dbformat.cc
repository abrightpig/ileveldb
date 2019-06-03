// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include <stdio.h>
#include "db/dbformat.h"
#include "port/port.h"
#include "util/coding.h"

namespace leveldb {

static uint64_t PackSequenceAndType(uint64_t seq, ValueType t) {
    assert(seq <= kMaxSequenceNumber);
    assert(t <= kValueTypeForSeek);
    return (seq << 8) | t;
}


void AppendInternalKey(std::string* result, const ParsedInternalKey& key) {
    result->append(key.user_key.data(), key.user_key.size());
    PutFixed64(result, PackSequenceAndType(key.sequence, key.type));
}

std::string ParsedInternalKey::DebugString() const {
    char buf[50];
    snprintf(buf, sizeof(buf), " @ %llu : %d",
            (unsigned long long) sequence,
            int(type));
    std::string result = "'";
    result += EscapeString(user_key.ToString());
    result += buf;
    return result;
}

LookupKey::LookupKey(const Slice& user_key, SequenceNumber s) {
    size_t usize = user_key.size();
    size_t needed = usize + 13;     // A conservative estimate
    char* dst;
    if (needed <= sizeof(space_)) {
        dst = space_;
    }
    else {
        dst = new char[needed];
    }
    start_ = dst;
    dst = EncodeVarint32(dst, usize + 8);
    kstart_ = dst;
    memcpy(dst, user_key.data(), usize);
    dst += usize;
    EncodeFixed64(dst, PackSequenceAndType(s, kValueTypeForSeek));
    dst += 8;
    end_ = dst;
}

}   // namespace leveldb

