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



privte:
    friend class Compaction;
    friend class VersionSet;

    class LevelFileNumIterator;

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

private:
    class Builder;

    friend class Compaction;
    friend class Version;

    uint64_t next_file_number_;     // ** to-catch: what file ?

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
