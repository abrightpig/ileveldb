// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_DBFORMAT_H_
#define STORAGE_LEVELDB_DB_DBFORMAT_H_

#include <stdio.h>
#include "leveldb/comparator.h"
#include "leveldb/db.h"
#include "leveldb/filter_policy.h"
#include "leveldb/slice.h"
#include "leveldb/table_builder.h"
#include "util/coding.h"
#include "util/logging.h"

namespace leveldb {

// Grouping of constants. We may want to make some of these
// parameters set via options.

namespace config {
static const int kNumLevels = 7;

// Level-0 compaction is started when we hit this many files.
static const int kL0_CompactionTrigger = 4;

// Soft limit on number of level-0 files. We slow down writes at this point.
static const int kL0_SlowdownWritesTrigger = 8;

// Maximum number of level-0 files. We stop writes at this point.
static const int kL0_StopWritesTrigger = 12;

// ** to-catch
// Maximum level to which a new compacted memtable is pushed if it 
// does not create overlap. We try to push to level 2 to avoid the
// relatively expensive level 0=>1 compactions and to avoid some 
// expensive manifest file operations. We do not push all the way to 
// the largest level since that can generate a lot of wasted disk
// space if the same key space is being repeatly overwritten.
static const int kMaxMemCompactLevel = 2;

// Approximate gap in bytes between samples of data read during iteration.
static const int kReadBytesPeriod = 1048576;

}   // namespace config

class InternalKey;

// Value types encoded as the last component of internal keys.
// DO NOT CHANGE THESE ENUM VALUES: they are embeded in the on-disk
// data structures.
enum ValueType {
    kTypeDeletion = 0x0,
    kTypeValue = 0x1
};
// ** to-catch
// kValueTypeForSeek defines the ValueType that should be passed when
// constructing a ParsedInternalKey object for seeking to a particular
// sequence number (since we sort sequence numbers in decreasing order
// and the value type is embedded as the low 8 bit s in the sequence 
// number in the internal keys, we need to use the highest-numbered
// ValueType, not the lowest).
static const ValueType kValueTypeForSeek = kTypeValue;

typedef uint64_t SequenceNumber;

// We leave eight bits empty at the bottom so a type and sequence#
// can be packed together into 64-bits.
static const SequenceNumber kMaxSequenceNumber = 
    ((0x1ull << 56) - 1);

struct ParsedInternalKey {
    Slice user_key;
    SequenceNumber sequence;
    ValueType type;

    ParsedInternalKey() {} // Intentionally left unitialized (for speed)
    ParsedInternalKey(const Slice& u, const SequenceNumber& seq, ValueType t)
        : user_key(u), sequence(seq), type(t) { }
    std::string DebugString() const;
};




// A comparator for internal keys that uses a specified comparator for 
// the user key portion and breaks ties by descending sequence number.
class InternalKeycomparator : public Comparator {
private:
    const Comparator* user_comparator_;

public:

    const Comparator* user_comparator() const { return user_comparator_; }
    
};  // class InternalKeyComparator









// Modules in this directory should keep internal keys wrapped inside
// the following class instead of plain strings so that we do not
// incorrectly use string comparisions instead of an InternalKeyComparator.
class InternalKey {
private:
    std::string rep_;   
public: 
    InternalKey() { }   // Leave rep_ as empty to indicate it is invalid
    InternalKey(const Slice& user_key, SequenceNumber s, ValueType t) {
        AppendInternalKey(&rep_, ParsedInternalKey(user_key, s, t)); 
    }


};  // class InternalKey



// A helper class useful for DBImpl::Get()
class LookupKey {
public:
    // Initialize *this for looking up user_key at a snapshot with
    // the specified sequence number.
    LookupKey(const Slice& user_key, SequenceNumber sequence);

    ~LookupKey();

    // Return a key suitable for lookup in a MemTable.
    Slice memtable_key() const { }



private:
    // We construction a char array of the form:
    //  klength varint32            <-- start_
    //  userkey char[klength]       <-- kstart_
    //  tag     uint64              
    //                              <-- end_
    //  The array is a suitable MemTable key. 
    //  The suffix starting with "userkey" can be used as an InternalKey.
    const char* start_;
    const char* kstart_;
    const char* end_;
    char space_[200];           // Avoid allocation for short keys

    // No copying allowed
    LookupKey(const LookupKey&);
    void operator=(const LookupKey&);
};  // class LookupKey

inline LookupKey::~LookupKey() {

}


}   // namespace leveldb

#endif  //  STORAGE_LEVELDB_DB_DBFORMAT_H_
