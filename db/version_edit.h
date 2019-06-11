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

struct FileMetaData {
    int refs;
    int allowed_seeks;              // Seeks allowed util compcation  **to-catch
    uint64_t number;
    uint64_t file_size;             // File size in bytes
    InternalKey smallest;           // Smallest internal key served by table
    InternalKey largest;            // Largest internal key served by table
    
    FileMetaData() : refs(0), allowed_seeks(1 << 30), file_size(0) { } 
};

class VersionEdit {
public:

    VersionEdit() { Clear(); } 
    ~VersionEdit() { }

    void Clear();
   
    void SetLogNumber(uint64_t num) {
        has_log_number_ = true;
        log_number_ = num;
    }
    void SetPrevLogNumber(uint64_t num) {
        has_prev_log_number_ = true;
        prev_log_number_ = num;
    }

private:
    friend class VersionSet;

    typedef std::set< std::pair<int, uint64_t> > DeletedFileSet;

    uint64_t log_number_;
    uint64_t prev_log_number_;
    uint64_t next_file_number_;

    bool has_log_number_;
    bool has_prev_log_number_;


    //std::vector< std::pair<int, InternalKey> > compact_pointers_;
    DeletedFileSet deleted_files_;
    std::vector< std::pair<int, FileMetaData> > new_files_;
};  // class VersionEdit

}   // namespace leveldb


#endif // STORAGE_LEVELDB_DB_VERSION_EDIT_H_
