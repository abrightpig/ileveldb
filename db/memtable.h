// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_DB_MEMTABLE_H_
#define STORAGE_LEVELDB_DB_DB_MEMTABLE_H_

#include <string>
#include "leveldb/db.h"
#include "db/dbformat.h"
#include "db/skiplist.h"
#include "util/arena.h"


namespace leveldb {

class InternalKeyComparator;
class Mutex;
class MemTableIterator;

class MemTable {
public:
    // MemTables are reference counted. The inital reference count
    // is zero and the caller must call Ref() at least once.
    explicit MemTable(const InternalKeyComparator& comparator);

    // Increase reference count.
    void Ref() { ++refs_; }

    // Drop reference cout. Delete if no more references exist.
    void Unref() {
        --refs_;
        assert(refs_ >= 0);     // ** to-catch: why is there an assert??
        if (refs_ <= 0) {
            delete this;
        }
    }

    // Returns an estimate of the number of bytes of data in use by this
    // data structure. It is safe to call when MemTable is being modified.
    size_t ApproximateMemoryUsage();



    // Add an entry into memtable that maps key to value at the 
    // specified sequence number and with the specified type.
    // Typically value will be empty if type==kTypeDeletion.
    void Add(SequenceNumber seq, ValueType type,
            const Slice& key,
            const Slice& value);

    // If memtable contains a value for key, store it in *value and return true.
    // If memtable contains a deletion for key, store a NotFound() error
    // in *status and return true.
    // Else, return false.
    bool Get(const LookupKey& key, std::string* value, Status* s);

private:
    ~MemTable();    // Private since only Unref() should be used to delete it

    struct KeyComparator {
        const InternalKeyComparator comparator;
        explicit KeyComparator(const InternalKeyComparator& c) : comparator(c) { }
        int operator()(const char* a, const char* b) const;
    };


    typedef SkipList<const char*, KeyComparator> Table;

    KeyComparator comparator_;
    int refs_;
    Arena arena_;
    Table table_;

    // No Copying allowed
    MemTable(const MemTable&);
    void operator=(const MemTable&);
};  // class MemTable



}   // namespace leveldb




#endif  //  STORAGE_LEVELDB_DB_DB_MEMTABLE_H_
