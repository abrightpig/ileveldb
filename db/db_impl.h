// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_H_

#include <deque>
#include <set>
#include "db/dbformat.h"
#include "db/log_writer.h"
#include "db/snapshot.h"
#include "leveldb/db.h"
#include "leveldb/env.h"


namespace leveldb {

class MemTable;
class TableCache;
class Version;
class VersionEdit;
class VersionSet;

class DBImpl : public DB {
public:
    DBImpl(const Options& options, const std::string& dbname);   
    virtual ~DBImpl();

    // Implementations of the DB interface
    virtual Status Put(const WriteOptions&, const Slice& key, const Slice& value);
    virtual Status Delete(const WriteOptions&, const Slice& key);
    virtual Status Write(const WriteOptions&, WriteBatch* updates);
    virtual Status Get(const ReadOptions& options,
                        const Slice& key,
                        std::string* value);
    virtual Iterator* NewIterator(const ReadOptions&);
    virtual const Snapshot* GetSnapshot();
    virtual void RealeaseSnapshot(const Snapshot* snapshot);
    virtual bool GetProperty(const Slice& property, std::string* value);
    virtual void GetAppoximateSizes(const Range* range, int n, uint64_t* sizes);
    virtual void CompactRange(const Slice* begin, const Slice* end);

    // **to-add : TEST_METHODS

    // **to-catch
    void RecordReadSample(Slice key);


private:
    friend class DB;
    struct CompactionState;
    struct Writer;

    Iterator* NewInternalIterator(const ReadOptions&, 
                                SequenceNumber* latest_snapshot,
                                uint32_t* seed);

    Status NewDB();

    // Constant after construction
    Env* const env_;
    //
    //
    const Options options_; 
    bool owns_info_log_;
    bool owns_cache_;
    const std::string dbname_;

    // table_cache_ provides its own synchronization
    TableCache* table_cache_;

}




}   // namespace leveldb


#endif  // STORAGE_LEVELDB_DB_DB_IMPL_H_

