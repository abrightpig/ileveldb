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

private:
    ~MemTable();    // Private since only Unref() should be used to delete it


    int refs_;

    // No Copying allowed
    MemTable(const MemTable&);
    void operator=(const MemTable&);
};  // class MemTable



}   // namespace leveldb




#endif  //  STORAGE_LEVELDB_DB_DB_MEMTABLE_H_
