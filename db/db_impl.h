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

    // Recover the descriptor from the presistent storage. May do a significant
    // amount of work to recover recently logged updates. Any changes to 
    // be made to the descriptor are added to *edit.
    Status Recover(VersionEdit* edit, bool* save_manifest)
        EXCLUSIVE_LOCKS_REQUIRED(mutex_);
       

    // Constant after construction
    Env* const env_;
    const InternalKeyComparator internal_comparator_;
    const InternalFilterPolicy internal_filter_policy_;
    const Options options_; 
    bool owns_info_log_;
    bool owns_cache_;
    const std::string dbname_;

    // table_cache_ provides its own synchronization
    TableCache* table_cache_;

    // Lock over the persistent DB state. Non-NULL iff successfully acquired.
    FileLock* db_lock_;

    // State below is protected by mutex_
    port::Mutex mutex_;
    port::AtomicPointer shutting_down_;     // **to-catch: why use this name??
    port::CondVar bg_cv_;           // signaleled when background work finishes
    MemTable* mem_;
    MemTable* imm_;                 // Memtable being compacted
    port::AtomicPointer has_imm_;   // So bg thread can detect non-NULL imm_
    WritableFile* logfile_;
    uint64_t logfile_number_;
    log::Writer* log_;
    uint32_t seed_;                 // For sampling.

    // Queue of writers
    std::deque<Writer*> writer_;
    WriteBatch* tmp_batch_;

    SnapshotList snapshots_;
    


}




}   // namespace leveldb


#endif  // STORAGE_LEVELDB_DB_DB_IMPL_H_

