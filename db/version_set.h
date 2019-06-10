// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.
//
// The representation of a DBImpl consists of a set of Versions. The
// newest version is called "current". Older versions may be kept
// around to provide a consistent view to live iterators.
//
// Each Versions keeps track of a set of Table files per level. The
// entier set of versions is maintained in a VersionSet.
//
// Version, VersionSet are thread-compatible, but require external
// synchronization on all accesses.
//
#ifndef STORAGE_LEVELDB_DB_VERSION_SET_H_
#define STORAGE_LEVELDB_DB_VERSION_SET_H_

#include <map>
#include <set>
#include <vector>
#include "db/dbformat.h"
#include "db/version_edit.h"
#include "port/port.h"
#include "port/thread_annotations.h"

namespace leveldb {

namespace log { class Writer; }

class Compaction;
class Iterator;
class MemTable;
class TableBuilder;
class TableCache;
class Version;
class VersionSet;
class WritableFile;

class Version {
public:
    // ** to-catch
    // Append to *iters a sequence of iterators that will
    // yield the contents of this Version when merged together.
    // REQUIRES: This version has been saved (see VersionSet::SaveInfo)
    void AddIterators(const ReadOptions&, std::vector<Iterator*>* iters);

    // Lookup the value for key. If found, store it in *val and 
    // return OK. Else return a non-OK status. Fills *stats.
    // REQUIRES: lock is not held.
    struct GetStats {
        FileMetaData* seek_file;
        int seek_file_level;
    }
    Status Get(const ReadOptions&, const LookupKey& key, std::string* val,
                GetStats* stats);


    // Reference count management(so Versions do not disappear out from 
    // under live iterators)
    void Ref();

privte:
    friend class Compaction;
    friend class VersionSet;

    class LevelFileNumIterator;


    VersionSet* vset_;          // VersionSet to which this Version belongs
    Version* next_;             // Next version in linked list
    Version* prev_;             // Previous version in linked list
    int refs_;                  // Number of 

    // List of files per level
    std::vector<FileMetaData*> files_[config::kNumLevels];

    // Level that should be compacted next and its compaction score.
    // Score < 1 means compaction is not strictly needed. These fields
    // are initialized by Finalize().
    double compaction_score_;
    int compaction_level_;

    ~Version();

    // No copying allowed
    Version(const Version&);
    void operator=(const Version&);
};   // class Version

class VersionSet {
public:
    VersionSet(const std::string& dbname,
               const Options* options,
               TableCache* table_cache,
               const InternalKeyComparator*);
    ~VersionSet();

    // Allocate and return a new file number
    uint64_t NewFileNumber() { return next_file_number_++; }

    // Arrange to reuse "file_number" unless a newer file number has
    // already been allocated.
    // REQUIRES: "file_number" was returned by a call to NewFileNumber().
    void ReuseFileNumber(uint64_t file_number) {
        if (next_file_number_ == file_number + 1) {
            next_file_number = file_number;
        }
    }

    // Return the number of Table files at the specified level.
    int NumLevelFiles(int level) const;

    // Return the last sequence number.
    uint64_t LastSequence() const { return last_sequence_; }

    // Set the last sequence number to s.
    void SetLastSequence(uint64_t s) {
        assert(s >= last_sequence);
        last_sequence_ = s;
    }

    // Return the current log file number
    uint64_t LogNumber() const { return log_number_; }

    // Return the log file number for the log file that is currently
    // being compacted, or zero if there is no such a log file.
    uint64_t PreLogNumber() const { return prev_log_number_; }



    // Returns true iff some level needs a compaction.
    bool NeedsCompaction() const {
        Version* v = current_;
        return (v->compaction_score_ >= 1) || (v->file_to_compcact != NULL);
    }



private:
    class Builder;

    friend class Compaction;
    friend class Version;


    void Finalize(Version* v);



    Env* const env_;
    TableCache* const table_cache_;
    const InternalKeyComparator icmp_;
    uint64_t next_file_number_;     // ** to-catch: what file ?
    uint64_t last_sequence_;
    uint64_t log_number_;
    uint64_t pre_log_number_;   // 0 or backing store for memtable being compacted



    Version dummy_versions_;    // Head of circular doubly-linked list of versions.
    Version* current_;          // == dummy_versions_.prev_

    // No copying allowed
    VersionSet(const VersionSet&);
    void operator=(const VersionSet&)
};  // class VersionSet

// A compaction encapsulates information about a compaction.
class Compaction {
public:
    ~Compaction();

private:
    friend class Version;
    friend class VersionSet;

    Compaction(const Options* options, int level);

};  // class Compaction 

}   // leveldb


#endif  //  STORAGE_LEVELDB_DB_VERSION_SET_H_
