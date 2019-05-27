// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.
//
// WriteBatch::rep_ :=
//      sequence: fixed64
//      count: fixed32
//      data: record[count]
// record :=
//      kTypeValue varstring varstring
//      kTypeDeletion varstring
// varstring :=
//      len: varint32
//      data: uint8[len]


#include "leveldb/write_batch.h"

#include "leveldb/db.h"
#include "db/dbformat.h"
#include "db/memtable.h"
#include "db/write_batch_internal.h"
#include "util/coding.h"

namespace leveldb {

// WriteBatch header has an 8-byte sequence number followed by a 4-byte count.
static const size_t kHeader = 12;

WriteBatch::WriteBatch() {
    Clear();
}

WriteBatch::~WriteBatch() { } 

int WriteBatchInternal::Count(const WriteBatch* b) {
    return DecodeFixed32(b->rep_.data() + 8);
}

void WriteBatchInternal::SetCount(WriteBatch* b, int n) {
   EncodeFix32(&b->rep_[8], n); 
}

SequenceNumber WriteBatchInternal::Sequence(const WriteBatch* b) {
    return SequenceNumber(DecodeFixed64(b->rep_.data()));
}

void WriteBatchInternal::SetSequence(WriteBatch* b, SequenceNumber seq) {
    EncodeFix64(&b->rep_[0], seq);
}

void WriteBatchInternal::Append(WriteBatch* dst, const WriteBatch* src) {
    SetCount(dst, Count(dst) + Count(src));
    assert(src->rep_.size() >= kHeader);
    dst->rep_.append(src->rep_.data() + kHeader, src->rep_.size() - kHeader);
    // **to-catch: how to split string data without their size?
}

}   // namespace leveldb
