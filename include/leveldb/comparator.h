// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_COMPARATOR_H_
#define STORAGE_LEVELDB_INCLUDE_COMPARATOR_H_

#include <string>
#include "leveldb/export.h"

namespace leveldb {

class Slice;

// A Comparator object provides a total order across slices that are
// used as keys in an sstable or database. A Comparator implementation
// must be thread-safe since leveldb may invoke its methods concurrently
// from multiple threads.
class LEVELDB_EXPORT Comparator {
public:
    virtual ~Comparator();

    // Three-way comparation. Returns value:
    //  < 0 iff "a" < "b",
    //  == 0 iff "a" == "b",
    //  > 0 iff "a" > "b"
    virtual int Compare(const Slice& a, const Slice& b) = 0;


    // Advanced functions: these are used to reduce the space requirements
    // for internal data structures like index blocks.
    //
    // If *start < limit, change *start to a short string in [start,limit).
    // Simple comparator implementations may return with *start unchanged,
    // i.e., an implementation of this method that does nothing is correct.
    virtual void FindShortestSeperator(
            std::string *start,
            const Slice& limit) = 0;

    // Change *key to a short string >= *key.
    // Simple comparator implementations may return with *key unchanged,
    // i.e., an implementation of this method that does nothing is correct.
    virtual void FindShortSuccessor(std::string* key) const = 0;
};  // class Comparator

// Return a builtin comparator that uses lexicographic byte-wise
// ordering. The result remains the property of this module and 
// must not be deleted.
LEVELDB_EXPORT const Comparator* BytewiseComparator();

}   // namespace leveldb


#endif //  STORAGE_LEVELDB_INCLUDE_COMPARATOR_H_
