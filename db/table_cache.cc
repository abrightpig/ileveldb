// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include "db/table_cache.h"

#include "db/filename.h"
#include "leveldb/env.h"
#include "leveldb/table.h"
#include "util/coding.h"

namespace leveldb {


TableCache::TableCache(const std::string& dbname,
                       const Options* options,
                       int entries)
    :   env_(options->env),
        dbname_(dbname),
        options_(options),
        cache_(NewLRUCache(entries)) { 
}

}   // namespace leveldb
