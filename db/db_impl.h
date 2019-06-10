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
       

    
    void MaybeScheduleCompaction() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    static void BGWork(void* db);
    void BackgroundCall();
    void BackgroundCompaction() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    void CleanupCompaction(CompactionState* compact)
        EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    Status DoCompactionWork(CompactionState* compact)
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
    port::Mutex mutex_;                     // ** to-catch: where does mutex_ come from?
    port::AtomicPointer shutting_down_;     
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

    // Set of table files to protect from deletion because they are
    // part of ongoing compactions.
    std::set<uint64_t> pending_outputs_;

    // Has a background compaction been scheduled or is running?
    bool bg_compaction_scheduled_;

    // Information for a manual compaction
    struct ManualCompaction {
        int level;
        bool done;
        const InternalKey* begin;   // NULL means beginning of key range
        const InternalKey* end;     // NULL means end of key range
        InternalKey tmp_storage;    // Used to keep track of compaction process
    };
    ManualCompaction* manual_compaction_;

    VersionSet* versions_;

    // Have we encountered a background error in the paranoid mode?
    Status bg_error_;

    // Per level compaction stats. stats_[level] stores the stats for 
    // compactions that produced data for the specified "level".
    struct CompactionStats {
        int64_t micros;
        int64_t bytes_read;
        int64_t bytes_written;
        
        CompactionStats() : micros(0), bytes_read(0), bytes_written(0) { }

        void Add(const CompactionStats& c) {
            this->micros += c.micros;
            this->bytes_read += c.bytes_read;
            this->byte_written += c.bytes_written;
        }
    };
    CompactionStats stats_[config::kNumLevels];

    // No copying allowed
    DBImpl(const DBImpl&);
    void operator=(const DBImpl&);

    const Comparator* user_comparator() const {
        return internal_comparator_.user_comparator();
    }
};  // class DBImpl




}   // namespace leveldb


#endif  // STORAGE_LEVELDB_DB_DB_IMPL_H_

