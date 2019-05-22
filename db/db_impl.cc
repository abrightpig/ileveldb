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
{


}    

Status DB::Open(const Opetion& options, const std::string& dbname,
                DB** dbptr) {
    *dbptr = NULL;

    DBImpl* impl = new DBImpl(options, dbname);


}


}   // namespace leveldb
