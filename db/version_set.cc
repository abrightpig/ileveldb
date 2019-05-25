// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include "db/version_set.h"

#include <algorithm>
#include <stdio.h>
#include "db/filename.h"
#include "db/log_reader.h"
#include "db/log_writer.h"
#include "db/memtable.h"
#include "db/table_cache.h"
#include "leveldb/env.h"
#include "leveldb/table_builder.h"
#include "table/merger.h"
#include "table/two_level_iterator.h"
#include "util/coding.h"
#include "util/logging.h"

namespace leveldb {


VersionSet::VersionSet(const std::string& dbname,
                       const Option* options,
                       TableCache* table_cache,
                       const InternalKeyComparator* cmp)
    :   env_(options->env),
        dbname_(dbname),
        options_(options),
        table_cache_(table_cache),  // ** to-catch: what is relation between version_set and tablecache?
        icmp_(*cmp),
        next_file_number_(2),
        manifest_file_number_(0),   // Filled by Recover()
        last_sequence_(0),
        log_number_(0),
        // ** to-add



}   // namespace leveldb
