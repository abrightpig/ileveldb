// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_VERSION_EDIT_H_
#define STORAGE_LEVELDB_DB_VERSION_EDIT_H_

#include <set>
#include <utility>
#include <vector>
#include "db/dbformat.h"

namespace leveldb {

class VersionSet;


class VersionEdit {
public:

    VersionEdit() { Clear(); } 
    ~VersionEdit() { }

    void Clear();
   
    void SetLogNumber(uint64_t num) {
        has_log_number_ = true;
        log_number_ = num;
    }

private:
   friend class VersionSet;


};  // class VersionEdit

}   // namespace leveldb


#endif // STORAGE_LEVELDB_DB_VERSION_EDIT_H_
