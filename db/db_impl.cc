// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include "db/db_impl.h"

#include "algorithm"
#include "set"
#include "string"
#include "stdint.h"
#include "stdio.h"
#include "vector"
#include "db/builder.h"
#include "db/db_iter.h"
#include "db/dbformat.h"
#include "db/filename.h"
#include "db/log_reader.h"
#include "db/log_writer.h"
#include "db/memtable.h"
#include "db/table_cache.h"
#include "db/version_set.h"
#include "db/write_batch_internal.h"
#include "leveldb/db.h"
#include "leveldb/status.h"
#include "leveldb/table.h"
#include "leveldb/table_builder.h"
#include "port/port.h"
#include "table/block.h"
#include "table/merger.h"
#include "table/two_level_iterator.h"
#include "util/coding.h"
#include "util/logging.h"
#include "util/mutexlock.h"


namespace leveldb {

const int kNumNonTableCacheFiles = 10;


// Infomation kept for every waiting writer.
struct DBImpl::Writer {
    Status status;
    WriteBatch* batch;
    bool sync;
    bool done;
    port::CondVar cv;

    explicit Writer(port::Mutex* mu) : cv(mu) {}
};





options SanitizeOptions(const std::string& dbname,
                        const InternalKeyComparator* icmp,
                        const InternalFilterPolicy* ipolicy,
                        const Options& src) {
    // ** to-add
    Options result = src;
    //
    //
    //
    return result;
}


DBImpl::DBImpl(const Options& raw_options, const std::string dbname) 
    :   env_(raw_options.env),
        internal_comparator_(raw_options.comparator),
        internal_filter_policy_(raw_options.filter_policy),
        options_(SanitizeOptions(dbname, &internal_comparator_,
                                &internal_fiter_policy_, raw_options)),
        owns_info_log(options_.info_log != raw_options.info_log),
        own_cache_(options_.block_cache != raw_options.block_cache),
        dbname_(dbname), 
        db_lock_(NULL),
        shutting_down_(NULL),
        bg_cv_(&mutex_),
        mem_(NULL),
        imm_(NULL),
        logfile_(NULL),
        logfile_number_(0),
        log_(NULL),
        seed_(0),
        tmp_batch_(new WriteBatch),
        bg_compaction_scheduled_(false),
        manual_compaction_(NULL) {
    has_imm_.Release_Store(NULL);

    // ** to-catch: 1, what are other users? 2, only 10 file ?
    // Reserver ten files or so for other uses and give the rest to TableCache.
    const int table_cache_size = options_.max_open_files - kNumNonTableCacheFiles;
    table_cache_ = new TableCache(dbname_, &options_, table_cache_size); 

    versions_ = new VersionSet(dbname_, &options_, table_cache_, 
                                &internal_comparator_);
}    


void DBImpl::RecordBackgroundError(const Status& s) {
    mutext_.AssertHeld();
    if (bg_error_.ok()) {
        bg_error_ = s;
        bg_cv_.SignalAll();
    }
}

Status DBImpl::Get(const ReadOptions& options,
                   const Slice& key,
                   std::string* value) {
    Status s;
    MutexLock l(&mutex_);
    SequenceNumber snapshot;
    if (options.snapshot != NULL) {
        snapshot = reinterpret_cast<const SnapshotImpl*>(options.snapshot)->number_;
    }
    else {
        snapshot = version_->LastSequence();
    }

    MemTable* mem = mem_;
    MemTable* imm = imm_;
    Version* current = versions_->current();    // **to-catch
    mem->Ref();
    if (imm != NULL) imm->Ref();
    current->Ref();

    bool have_stat_update = false;
    Version::GetStats stats;

    // Unlock while reading from files and memtables  ** to-catch: why, how?
    {
        mutex_.Unlock();
        // First look in the memtable, then in the immutable memtable(if any).
        LookUpKey lkey(key, snapshot);
        if (mem->Get(lkey, value, &s)) {
            // Done
        }
        else if (imm != NULL && imm->Get(lkey, value, &s)) {
            // Done
        } 
        else {
            s = current->Get(options, lkey, value, &stats);
            have_stat_update = true;
        }
        mutex_.Lock();
    }



}



// Convenience methods
// ** to-catch: why DB::Put() is called here?
Status DBImpl::Put(const WriteOptions& o, const Slice& key, const Slice& value) {
    return DB::Put(o, key, val);
}


Status DBImpl::Write(const WriteOptions & options, WriteBatch* my_batch) {
    Writer w(&mutex_);
    w.batch = my_batch;
    w.sync = options.sync;
    w.done = false;

    MutexLock l(&mutex_);
    writers_.push_back(&w);
    while (!w.done && &w != writers_.front()) {
        w.cv.Wait();
    }
    if (w.done) {               // ** to-catch: why check w.done here??
        return w.status;
    }

    // May temporarily unlock and wait.
    Status status = MakeRoomForWrite(my_batch == NULL);
    uint64_t last_sequence = versions_->LastSequence();
    Writer* last_writer = &w;
    if (status.ok() && my_batch != NULL) {  // NULL batch is for compactions
        WriteBatch* updates = BuildBatchGroup(&last_writer);
        WriteBatchInternal::SetSequence(updates, last_sequence + 1);   
        last_sequence += WriteBatchInternal::Count(updates);
        
        // Add to log and apply to memtable. We can release the lock
        // during this phase since &w is currently responsible for logging 
        // and protects against concurrent loggers and concurrent writes
        // into mem_.
        {
            mutex_.Unlock();
            status = log_->AddRecord(WriteBatchInternal::Contents(updates));
            bool sync_error = false;
            if (status.ok() && options.sync) {
                status = logfile_->Sync(); 
                if (!status.ok()) {
                    sync_error = true;
                }
            }
            if (status.ok()) {
                status = WriteBatchInternal::InsertInto(updates, mem_);
            }
            mutex_.Lock(); 
            if (sync_error) {
                // The state of the log file is indeterminate: the log record we
                // just added may or may not show up when the DB is re-opened.
                // So we force the DB into a mode where all future writes fail.
                // ** to-catch
                RecordBackgroundError(status);
            }
        }
        if (updates == tmp_batch_) tmp_batch_->Clear();     // **to-catch

        versions_->SetLastSequence(last_sequence);
    }

    while (true) {
        Writer* ready = writers_.front();
        writers_.pop_front();
        if (ready != &w) {              //other threads' write request    
            ready->status = status;
            ready->done = true;
            ready->cv.Signal();
        }
        if (ready == last_writer) break; 
    }

    // Notify new head of write queue
    if (!writers_.empty()) {
        writers_.front()->cv.Signal();
    }

    return status;
}

// REQUIRES: Writer list must be non-empty
// REQUIRES: First writer must have a non-NULL batch
WriteBatch* DBImpl::BuildBatchGroup(Writer** last_writer) {
    assert(!writers_.empty());
    Writer* first = writers_.front();
    WriteBatch* result = first->batch;
    assert(result != NULL);

    size_t size = WriteBatchInternal::ByteSize(first->batch);
    
    // Allow the group to grow up to a maximum size, but if the
    // original write is small, limit the growth so we do not slow
    // down the small write too much.
    size_t max_size = 1 << 20;
    if (size <= (128<<10)) {
        max_size = size + (128<<10);
    }

    *last_writer = first;
    std::deque<Writer*>::iterator iter = writers_begin();
    ++iter; // Advance past "first"
    for (; iter != writers_.end(); ++iter) {
        Writer* w = *iter;
        if (w->sync && !first->sync) {
            // Do not include a sync write into a batch handled by a non-sync write.
            break;
        }

        if (w->batch != NULL) {         // ** to-catch: why do this check?
            size += WriteBatchInternal::ByteSize(w->batch);
            if (size > max_size) {
                // Do not make batch too big
                break;
            }

            // Append to *result
            if (result == first->batch) {
                // Switch to temporary batch instead of disturbing caller's batch 
                result = tmp_batch_;
                assert(WriteBatchInternal::Count(result) == 0);
                WriteBatchInternal::Append(result, first->batch);
            }
            WriteBatchInternal::Append(result, w->batch);
        }
        *last_writer = w;
    }
    return result;
}


// REQUIRES: mutex_ is held
// REQUIRES: this thread is currently at the front of the writer queue
Status DBImpl::MakeRoomForWrite(bool force) {
    mutex_.AssertHeld();
    assert(!writers_.empty());
    bool allow_delay = !force;          // ** to-catch: why use ! ?
    Status s;
    while (true) {
        if (!bg_error_.ok()) {
            // Yield previous error
            s = bg_error_;
            break;
        } 
        else if (allow_delay && 
                versions_->NumLevelFiles(0) >= config::kL0_SlowdownWritesTrigger) {
            // We are getting close to hitting a hard limit on the number of
            // L0 files. Rather than delaying a single write by several 
            // seconds when we hit the hard limit, start delaying each
            // individual write by 1ms to reduce latency variance. Also,
            // this delay hands over some CPU to the compaction thread in 
            // case it is sharing the same core as the writer.
            mutex_.Unlock();
            env_SleepForMicroseconds(1000);
            allow_delay = false;    // Do not delay a single write more than once
            mutex_.Lock();
        }
        else if (!force && 
                 (mem_->AppoximateMemoryUsage() <= options.writer_buffer_size())) {
            // There is room in current memtable
            break;
        }
        else if (imm_ != NULL) {
            // We have filled up the current memtable, but the previous
            // one is still being compacted, so we wait.
            Log(options_.info, "Current memtable full; waiting... \n");
            bg_cv_.Wait();
        }
        else if (version_->NumLevelFiles(0) >= config::kL0_StopWritesTrigger) {
            // There are too many level-0 files.
            // ** to-catch: where and how do compact?
            Log(options_.info_log, "Too many L0 files; waiting...\n");
            bg_cv_.Wait();
        }
        else {
            // Attempt to switch to a new memtable and trigger compaction of old
            assert(version_->PrevLogNumber() == 0);
            uint64_t new_log_number = version_->NewFileNumber();
            WritableFile* lfile = NULL;
            s = env_->NewWritableFile(LogFileName(dbname_, new_log_number), &lfile);
            if (!s.ok()) {
                // Avoid chewing through file number space in a tight loop
                versions_->ReuseFileNumber(new_log_number);
                break;
            }
            delete log_;
            delete logfile_;
            log_file_ = lfile;
            logfile_number_ = new_log_number;
            log_ = new log::Writer(lfile);
            imm_ = mem_;
            has_imm_.Release_Store(imm_);
            mem_ = new MemTable(internal_comparator_);
            mem_->Ref();
            force = false;  // Do not force another compaction if have room ** to-catch
            MaybeScheduleCompaction();
        }
    }
    return s;
} 



// Default implementations of convenince methods that subclassses of DB
// can call if they wish
Status DB::Put(const WriteOptions& opt, const Slice& key, const Slice& value) {
    WriteBatch batch;
    batch.Put(key, value);
    return Write(opt, &batch);
}


DB::~DB() { }

Status DB::Open(const Opetion& options, const std::string& dbname,
                DB** dbptr) {
    *dbptr = NULL;

    DBImpl* impl = new DBImpl(options, dbname);
    impl->mutex_.Lock();
    VersionEdit edit;
    // Recover handles create_if_missing, error_if_exists
    bool save_manifest = false;
    Status s = impl->Recover(&edit, &save_manifest);
    if (s.ok() && impl->mem_ == NULL) {
        // Create new log and a corresponding memtable.
        uint64_t new_log_number = impl->versions_->NewFileNumber();
        WritableFile* lfile;
        s = options.env->NewWritableFile(LogFileName(dbname, new_log_number),
                                        &lfile);
        if (s.ok()) {
            edit.SetLogNumber(new_log_number);
            impl->logfile_ = lfile;
            impl->logfile_number_ = new_log_number;
            impl->log_ = new log::Writer(lfile);
            impl->mem_ = new MemTable(impl->internal_comparator_);
            impl->mem_->Ref();
        }
    }
    if (s.ok() && save_manifest) {
        edit.SetPrevLogNumber(0);   // No older logs needed after recovery.
        //****************************
        //****************************
    }
        //****************************
        //****************************
    impl->mutex_.Unlock();
    if (s.ok()) {
        assert(impl->mem_ != NULL);
        *dbptr = impl;
    }
    else {
        delete impl;
    }
    return s;
}


}   // namespace leveldb
