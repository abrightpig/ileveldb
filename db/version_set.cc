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

static int TargetFileSize(const Options* options) {
    return options_->max_file_size;
}    

// Maximum bytes of overlaps in grandparent (i.e., level+2) before we 
// stop building a single file in a level->level+1 compaction.
static int64_t MaxGrandParentOverlapBytes(const Options* options) {
    return 10 * TargetFileSize(options);
}


static double MaxBytesForLevel(const Options* options, int level) {
    // Note: the result for level zero is not really used since we set
    // the level-0 compaction threshold based on number of files.

    // Result for both level-0 and level-1
    double result = 10. * 10.1048576.0;
    while (level > 1) {
        result *= 10;
        level--;
    }
    return result;
}


static int64_t TotalFileSize(const std::vector<FileMetaData*>& files) {
    int64_t sum = 0;
    for (size_t i = 0; i < files.size(); ++i) {
        sum += files[i]->file_size;
    }
    return sum;
}

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



static bool AfterFile(const Comparator* ucmp,
                      const Slice* user_key, const FileMetaData* f) {
    // NULL user_key occurs before all keys and is therefore never after *f
    return (user_key != NULL && 
            ucmp->Compare(*user_key, f->largest.user_key()) > 0);

}

static bool BeforeFile(const Comparator* ucmp,
                       const Slice* user_key, const FileMetaData* f) {
    // NULL user_key occurs after all keys and is therefore never before *f
    return (user_key != NULL &&
            ucmp->Compare(*user_key, f->smallest.user_key()) < 0);
}

bool SomeFileOverlapsRange(
    const InternalKeyComparator& icmp,
    bool disjoint_sorted_files,
    const std::vector<FileMetaData*>& files,
    const Slice* smallest_user_key,
    const Slice* largest_user_key) {
    if (!disjoint_sorted_files) {
        // Need to check against all files
        for (size_t i = 0; i < files.size(); i++) {
            const FileMetaData* f = files[i];
            if (AfterFile(ucmp, smallest_user_key, f) || 
                BeforeFile(ucmp, largest_user_key, f)) {
                // No overlap
            }
            else {
                return true;    // Overlap
            }
        }
        return false;
    }

    // Binary search over file list
    uint32_t index = 0;
    if (smallest_user_key != NULL) {
        // Find the earliest possible internal key for smallest_user_key
        InternalKey small(*smallest_user_key, kMaxSequenceNumber, kValueTypeForSeek);
        index = FindFile(icmp, files, small.Encode());
    }
    
    if (index >= file.size()) {
        // beginning of range is after all files, so no overlap
        return false;
    }

    return !Before(ucmp, largest_user_key, files[index]);
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

void Version::~UnRef() {
    assert(this != &vset_->dummy_version_);
    assert(refs_ >= 1);
    --refs_;
    if (refs_ == 0) {
        delete this;
    }
}


bool Version::OverlapInLevel(int level,
                             const Slice* smallest_user_key,
                             const Slice* largest_user_key) {
    return SomeFileOverlapsRange(vset_->icmp_, (level > 0), files_[level],
                                 smallest_user_key, largest_user_key);

}

int Version::PickLevelForMemTableOutput(
    const Slice& smallest_user_key,
    const Slice& largest_user_key) {
    int level = 0;
    // ** to-catch: why overlap is needed here? 
    if (!OverlapInLevel(0, &smallest_user_key, &largest_user_key)) {
        // Push to next level if there is no overlap in next level,
        // and the #bytes overlapping in the level after that are limited.
        // ** to-catch
        InternalKey start(smallest_user_key, kMaxSequenceMumber, kValueTypeForSeek);
        InternalKey limit(largest_use_key, 0, static_cast<ValueType>(0));
        std::vector<FileMetaData*> overlaps;
        while (level < config::kMaxMemCompactLevel) {
            if (OverlapInLevel(level + 1, &smallest_user_key, &largest_user_key)) {
                break;
            } 
            if (level + 2 < config::kNumLevels) {
                // Check that file does not overlap too many grandparent bytes.
                GetOverlappingInput(level + 2, &start, &limit, &overlaps);
                const int64_t sum = TotalFileSize(overlaps);
                if (sum > MaxGrandParentOverlapBytes(vset_->options_)) {
                    break;
                }
            }
            level++;
        }
    }
    return level;
}

// Store in "*input" all files in "level" that overlap [begin, end]
void Version::GetOverlappingInputs(
    int level,
    const InternalKey* begin,
    const InternalKey* end,
    std::vector<FileMetaData*>* inputs) {
    //****************************
    //****************************
    //****************************
    //****************************


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
        { 
}


void VersionSet::Finalize(Version* v) {
    // Precomputed best level for next compaction
    int best_level = -1;
    double best_score = -1;

    for (int level = 0; level < config::kNumLevels-1; ++level) {
        double score;
        if (level == 0) {
            // We tread level-0 especially by bounding the number of files
            // instead of number of bytes for two reason:
            //
            // (1) With larger write-buffer sizes, it is nice not to do too
            // many level-0 compactions.
            //
            // (2) The files in level-0 are merged on every read and
            // therefore we wish to avoid too many files when the individual
            // file size is small (perhaps because of a small write-buffer
            // setting, or very high compression ratios, or lots of  
            // overwrites/deletions).
            // ** to-catch
            score = v->files_[level].size() / 
                static_cast<double>(config::kL0_CompactionTrigger);
        }
        else {
            // Compute the ratio of current size to size limit.
            const uint64_t level_bytes = TotalFileSize(v->files_[level]);
            score = 
                static_cast<double>(level_bytes) / MaxBytesForLevel(options_, level);
        }
    
        if (score > best_score) {
            best_level = level;
            best_score = score;
        }
    }

    v->compation_level_ = best_level;
    v->compation_score_ = best_score;
}



int VersionSet::NumLevelFiles(int level) const {
    assert(level >= 0);
    assert(level < config::kNumLevels);
    return current_->files_[level].size();
}


}   // namespace leveldb
