// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_DB_SNAPSHOT_H_
#define STORAGE_LEVELDB_DB_DB_SNAPSHOT_H_

#include "db/dbformat.h"
#include "leveldb/db.h"

namespace leveldb {
    
class SnapshotList;

// Snapshots are keyt in doubly-linked list in the DB.
// Each SnapshotImpl corresponds to a particular sequence number.
class SnapshotImpl : public Snapshot {
public:
    SequenceNumber number_; // const after creation

private:
    friend class SnapshotList;

    // SnapshotImpl is kept in a doubly-linked circular list
    SnapshotImpl* prev_;
    SnapshotImpl* next_;

    SnapshotList* list_;        // just for sanity checks
}

class SnapshotList {
public:
    SnapshotList() {
        list_.prev = &list_;
        list_.next = &list_;
    }

    bool empty() const { return list_.next = &list_; }
    SnapshotImpl* oldest() const { assert(!empty()); return list_.next_; }
    SnapshotImpl* newest() const { assert(!empty()); return list_.prev_; }

    const SnapshotImpl* New(SequenceNumber seq) {
        // ** to-catch
        SnapshotImpl* s = new SnapshotImpl;
        s->number_  = seq;
        s->list_ = this;
        s->next_ = &list_;
        s->pre_  = list_.prev_;
        s->prev_->next = s;
        return s;
    }

    void Delete(const SnapshotImpl* s) {
        // ** to-add
    
    }


private:
    // Dummy head of doubly-linked list of snapshots
    SnapshotImpl list_;
};

}  // namespace leveldb 

#endif  //  STORAGE_LEVELDB_DB_DB_SNAPSHOT_H_
