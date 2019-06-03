// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include <algorithm>
#include <stdint.h>
#include "leveldb/comparator.h"
#include "leveldb/slice.h"
#include "port/port.h"
#include "util/logging.h"

namespace leveldb {

Comparator::~Comparator() { }

namespace {
class BytewiseComparatorImpl : public Comparator {
public:
    BytewiseComparatorImpl() { }


    virtual int Compare(const Slice& a, const Slice& b) const {
        return a.compare(b);
    }

private:

}


}   // namespace

static port::OnceType once = LEVELDB_ONCE_INIT;
static const Comparator* bytewise;

static void InitModule() {
    bytewise = new BytewiseComparatorImpl;
}

const Comparator* BytewiseComparator() {
    port::InitOnce(&once, InitModule);
    return bytewise;
}

}   // namespace leveldb
