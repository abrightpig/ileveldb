// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.
//
// A database can be configured with a custom FilterPolicy object.
// This object is responsible for creating a small filter from a set
// of keys. These filters are stored in leveldb and are consulted
// automatically by leveldb to decide whether or not to read some
// infomation from disk. In many cases, a filter can cut down the
// number of disk seeks from a handful to a single disk seek per
// DB::Get() call.
//
// Most people will want to use the builtin bloom filter support (see
// NewBloomFilterPolicy() below).

#ifndef STORAGE_LEVELDB_INCLUDE_FILTER_POLICY_H_
#define STORAGE_LEVELDB_INCLUDE_FILTER_POLICY_H_

#include <string>
#include "leveldb/export.h"

namespace leveldb {

class Slice;

class LEVELDB_EXPORT FilterPolicy {
public:
    virtual ~FilterPolicy();

    // Return the name of this policy. Note that if the filter encoding
    // changes in an incompatible way, the same returned by this method
    // must be changed. Otherwise, old incompatible filters may be
    // passed to methods of this type.
    virtual const char* Name() const = 0;

    // keys[0,n-1] contains a list of keys (potentially with duplicates)
    // that are ordered according to the user supplied comparator.
    // Append a filter that summarize keys[0,n-1] to *dst.
    //
    // Warning: do not change the initial contents of *dst. Instead,
    // append the newly constructed filter to *dst.
    virtual void CreateFilter(const Slice* keys, int n, std::string* dst)
        const = 0;

    // "filter" contains the data appended by a preceding call to
    // CreateFilter() on this class. This method must return ture if
    // the key was in the list of keys passed to CreateFilter().
    // This method may return true or false if the key was not on the
    // list, but it should aim to return false with a high probability.
    //  ** to-catch: can return true if key exist ??
    virtual bool KeyMayMatch(const Slice& key, const Slice& filter) const = 0;

};  // class FilterPolicy

// Return a new filter policy that uses a bloom filter with approximately
// the specified number of bits per key. A good value for bits_per_key
// *********************** 
LEVELDB_EXPORT const FilterPolicy* NewBloomFilterPolicy(int bits_per_key);

}   // namespace leveldb

#endif  //  STORAGE_LEVELDB_INCLUDE_FILTER_POLICY_H_