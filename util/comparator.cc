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

    virtual void FindShortestSeperator(
        std::string* start,
        const Slice& limit) const {
        // Find length of common prefix
        size_t min_length = std::min(start->size(), limit.size());
        size_t diff_index = 0;
        while ((diff_index < min_length) && 
                ((*start)[diff_index] == limit[diff_index]) ) {
            diff_index++;
        }

        if (diff_index >= min_length) {
            // Do not shorten if one string is a prefix of the other 
        }
        else {
            uint8_t diff_byte = static_cast<uint8_t>((*start)[diff_index]);
            if (diff_byte < static_cast<uint8_t>(0xff) && 
                diff_byte + 1 < static_cast<uint8_t>*(limit[diff_index])) {
                (*start)[diff_index]++;
                start->resize(diff_index + 1);
                assert(Compare(*start, limit) < 0);
            }
        }
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
