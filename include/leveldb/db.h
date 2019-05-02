// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_DB_H_
#define STORAGE_LEVELDB_INCLUDE_DB_H_

#include <stdint.h>
#include <stdio.h>
#include "leveldb/export.h"
#include "leveldb/iterator.h"
#include "leveldb/options.h"

namespace leveldb {
    



// A DB is a persistent ordered map from keys to values.
// A DB is safe for concurrent access from multiple threads without
// any external synchronization.
class LEVELDB_EXPORT DB {
    public:
    // Open the database with the specified "name".
    // Stores a pointer to a heap-allocated database in *dbpter and returns
    // OK on success.
    // Stores NULL in *dbptr and returns a non-OK status on error.
    // Caller should delete *dbptr when it is no longer needed.
    static Status Open(const Options& options,
                       const std::string& name,
                       DB** dbptr);
    DB() { }
    virtual ~DB();

    // Set the database entry for "key" to "value". Returns OK on success,
    // and a non-OK status on error.
    // Note: consider setting options.sync = true.
    virtual Status Put(const WriteOptions& options,
                       const Slice& key,
                       const Slice& value) = 0;


    // Remove the database entry (if any) for "key". 
    // Returns OK on sucess, and a non-OK status on error. 
    // It is not an error if "key" did not exist in the databases.
    // Note: consider setting options.sync = true.
    virtual Status Delete(const WriteOptions& options, const Slice& key) = 0;

    // Apply the specifid updates to the database.
    // Returns OK on successs, non-OK on failure.
    // Note: consider setting options.sync = true.
    virtual Status Write(const WriteOptions& options, WriteBatch* updates) = 0;

    // If the database contains an entry for "key", store the corresponding value in *value and return OK.
    // If there is no entry for "key", leave *value unchanged 
    // and return a status for which Status::IsNotFound() return true.
    // May return some other status on an error.
    virtual Status Get(const ReadOptions& options, const Slice& key, std::string* value) = 0;

    // Return a heap-allocated iterator over the contents of the database.
    // The result of NewIterator() is initially invalid(caller must
    // call one of the Seek methods on the iterator before using it).
    //
    // Caller should delete the iterator when it is no longer needed.
    // The returned iterator should be deleted before this db is deleted.
    virtual Iterator* NewIterator(const ReadOptions& options) = 0;

    // Return a handle to the current DB state. Iterators created with
    // this handle will all observe a stable snapshot of the current DB state.
    // The caller must call ReleaseSnapshot(result) when the snapshot is no longer needed.
    virtual const Snapshot* GetSnapshot() = 0;


































}

}
