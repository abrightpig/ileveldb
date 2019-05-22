// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.
//
// Thread-safe(provide internal synchronization)
// ** to-catch : how to accomplish this?

#ifndef STORAGE_LEVELDB_DB_DB_CACHE_H_
#define STORAGE_LEVELDB_DB_DB_CACHE_H_

#include <string>
#include <stdint.h>
#include "db/dbformat.h"
#include "leveldb/cache.h"
#include "leveldb/table.h"
#include "port/port.h"


namespace leveldb {

class Env;

class TableCache {
public:
    TableCache(const std::string&dbname, const Options* options, int entries);
    ~TableCache();

    // ** to-add
    
    // Return an iterator for the specified file number (the corresponding
    // file length must be exactly "file_size" bytes). If "tableptr" is 
    // non-NULL, also sets "*tableptr" to point to the Table object
    // underlying the returned iterator, or NULL if no Table object underlies
    // the returned iterator. The returned "*tableptr" object is owned by
    // the cache and should not be deleted, and is valid for as long as the
    // returned iterator is live.
    Iterator* NewIterator(const ReadOptions& options,
                        uint64_t file_number,
                        uint64_t file_size,
                        Table** tableptr = NULL);
    


};  // class TableCache


}




#endif  // STORAGE_LEVELDB_DB_DB_CACHE_H_ 

