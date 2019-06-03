// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include "db/memtable.h"
#include "db/dbformat.h"
#include "leveldb/comparator.h"
#include "leveldb/env.h"
#include "leveldb/iterator.h"
#include "util/coding.h"

namespace leveldb {


MemTable::MemTable(const InternalKeyComparator& cmp)
    :   comparator_(cmp),
        refs_(0),
        table_(comparator_, &arena_) {
}

MemTable::~MemTable() {
    assert(refs_ == 0);
}

size_t MemTable::ApproximateMemoryUsage() { return arena_.MemoryUsage(); }






void MemTable::Add(SequenceNumber s, ValueType type,
                   const Slice& key,
                   const Slice& value) {
    // Format of an entry is concatenation of:
    // key_size     : varint32 of internal_key.size()
    // key bytes    : char[internal_key.size()]
    // value_size   : varint32 of value.size()
    // value bytes  : char[value.size()]
    size_t key_size = key.size();
    size_t val_size = value.size();
    size_t internal_key_size = key_size + 8;    
    const size_t encoded_len = 
        VarintLength(internal_key_size) + internal_key_size +
        VarintLength(val_size) + val_size;
    char* buf = arena_.Allocate(encoded_len);
    char* p = EncodeVarint32(buf, internal_key_size);
    memcpy(p, key.data(), key_size);
    p += key_size;
    EncodeFixed64(p, (s << 8) | type);
    p += 8;
    p = EncodeVarint32(p, val_size);
    memcpy(p, value.data(), val_size);
    assert((p + val_size) - buf == encoded_len);
    table_.Insert(buf);
}

void MemTable::Get(const LookupKey& key, std::string* value, Status* s) {
    Slice memkey = key.memtable_key();
    // ** to-catch: where is memkey parsed?
    Table::Iterator iter(&table);
    iter.Seek(memkey.data());
    if (iter.Valid()) {
    
    }
    return false;
}

}   // namespace leveldb
