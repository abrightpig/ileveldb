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

int FindFile(const InternalKeyComparator& icmp,
             const std::vector<FileMetaData*>& files,
             const Slice& key) {
    uint32_t left = 0;
    uint32_t right = files.size();
    while (left < right) {
        uint32_t mid = (left + right) / 2;
        const FileMetaData* f = files[mid];
        if (icmp.InternalKeyCompartor::Compare(f->largest.Encode(), key) < 0) {
            // Key at "mid.largest" is < "target". Therefore all
            // files at or before "mid" are uninteresting.
            left = mid + 1;
        }
        else {
            // Key at "mid.largeest" is >= "target". Therefore all files
            // after "mid" are uninteresting.
            right = mid;
        }
    }
    return right;
}

// Callback from TableCache::Get()
namespace {
enum SaverState {
    kNotFound,
    kFound,
    kDeleted,
    kCorrupt,
};
struct Saver {
    SaverState state;
    const Comparator* ucmp;
    Slice user_key;
    std::string* value;
};
}
static void SaveValue(void* arg, const Slice& ikey, const Slice& v) {
    Saver* s = reinterpret_cast<Saver*>(arg);
    ParsedInternalKey parsed_key;
    if (!ParsedInternalKey(ikey, &parsed_key)) {
        s->state = kCorrupt;
    }
    else {
        if (s->ucmp->Compare(parsed_key.user_key, s->user_key) == 0) {
            s->state = (parsed_key.type == kTypeValue) ? kFound : kDeleted;
            if (s->state == kFound) {
                s->value->assign(v.data(), v.size());
            }
        }
    }
}

static bool NewestFirst(FileMetaData* a, FileMetaData* b) {
    return a->number > b->number;
}


Status Version::Get(const ReadOptions& options,
                    const LookupKey& k,
                    std::string* value,
                    GetStats* stats) {
    Slice ikey = k.internal_key();
    Slice user_key = k.user_key();
    const Comparator* ucmp = vset_->icmp_.user_comparator();
    Status s;

    stats->seek_file = NULL;
    stats->seek_file_level = -1;
    FileMetaData* last_file_read = NULL;
    int last_file_read_level = -1;

    // We can search level-by-level since entries never hop across
    // levels. Therefore we are guaranteed that if we find data
    // in a smaller level, later levels are irrelevant.
    std::vector<FileMetaData*> tmp;
    FileMetaData* tmp2;
    for (int level = 0; level < config::kNumLevels; level++) {
        size_t num_files = files_[level].size();
        if (num_files == 0) continue;

        // Get the list of files to search in this level
        FileMetaData* const* files = &files_[level][0];
        if (level == 0) {
            // Level-0 files may overlap each other. Find all files that
            // overlap user_key and process them in order from newest to oldest.
            tmp.reserve(num_files);
            for (uint32_t i = 0; i < num_files; i++) {
                FileMetaData* f = files[i];
                if (ucmp->Compare(user_key, f->smallest.user_key()) >= 0 &&
                    ucmp->Compare(user_key, f->largest.user_key() <=0) {
                    tmp.push_back(f);
                }
            }
            if (tmp.empty()) continue; 
            std::sort(tmp.begin(), tmp.end(), NewestFirst);
            files = &tmp[0];
            num_files = tmp.size();
        } 
        else {
            // Binary search to find earliest index whose largest key >= ikey.
            uint32_t index = FindFile(vset_->icmp_, files_[level], ikey);
            if (index >= num_files) {
                file = NULL;
                num_files = 0;
            }
            else {
                tmp2 = files[index];
                if (ucmp->Compare(user_key, tmp2->smallest.user_key()) < 0) {
                    // All of "tmp2" is past any data for user_key
                    files = NULL;
                    num_files = 0;
                }
                else {
                    files = & tmp2;
                    num_files = 1;
                }
            }
        }

        for (uint32_t i = 0; i < num_files; ++i) {
            if (last_file_read != NULL && stats->seek_file == NULL) {
                // We have had more than one seek for this read. Charge the 1st file.
                stats->seek_file = last_file_read;
                stats->seek_file_level = last_file_read_level;
            }

            FileMetaData* f = files[i];
            last_file_read = f;
            last_file_read_level = level;

            Saver saver;
            saver.state = kNotFound;
            saver.ucmp = ucmp;
            saver.user_key = user_key;
            saver.value = value;
            s = vset_->table_cache_->Get(options, f->number, f->file_size,
                                         ikey, &saver, SaveValue);

            if (!s.ok()) {
               return s; 
            }
            switch (saver.state) {
                case kNotFound:
                    break;      // Keep searching in other files
                case kFound:
                    return s;
                case kDeleted:
                    s = Status::NotFound(Slice());  // Use empty error message for speed.
                    return s;
                case kCorrupt:
                    s = Status::Corruption("corrupted key for ", user_key);
                    return s;
            }
        }
    }

    return Status::NotFound(Slice());  // Use an empty error message for speed
}

void Version::Ref() {
    ++refs_;
}

VersionSet::VersionSet(const std::string& dbname,
                       const Option* options,
                       TableCache* table_cache,
                       const InternalKeyComparator* cmp)
    :   env_(options->env),
        dbname_(dbname),
        options_(options),
        table_cache_(table_cache),  // ** to-catch: what is relation between version_set and tablecache?
        icmp_(*cmp),
        next_file_number_(2),       // ** to-catch: why init as 2, not 1 ?
        manifest_file_number_(0),   // Filled by Recover()
        last_sequence_(0),
        log_number_(0),
        // ** to-add 
        { }

int VersionSet::NumLevelFiles(int level) const {
    assert(level >= 0);
    assert(level < config::kNumLevels);
    return current_->files_[level].size();
}


}   // namespace leveldb
