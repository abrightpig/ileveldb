// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include "db/db_impl.h"


namespace leveldb {


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

// Convenience methods
// ** to-catch: why DB::Put() is called here?
Status DBImpl::Put(const WriteOptions& o, const Slice& key, const Slice& value) {
    return DB::Put(o, key, val);
}


Status DBImpl::Write(const WriteOptions) {


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
