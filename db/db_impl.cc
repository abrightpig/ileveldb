// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include "db/db_impl.h"


namespace leveldb {

DBImpl::DBImpl(const Options& raw_options, const std::string dbname) 
    :   env_(raw_options.env),
        
{


}    

Status DB::Open(const Opetion& options, const std::string& dbname,
                DB** dbptr) {
    *dbptr = NULL;

    DBImpl* impl = new DBImpl(options, dbname);


}


}   // namespace leveldb
