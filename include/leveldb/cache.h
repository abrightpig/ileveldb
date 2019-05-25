// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.
//
// A cache is an interface that maps keys to values. It has internal
// synchronization and may be safely accessed concurrently from 
// multiple threads. It may automatically evict entries to make room
// for new entries. Values have a specified charge against the cache
// capacity. For example, a cache where the values are variable
// length strings, may use the length of the string as the charge for
// the string.
//
// A builtin cache implementation with a least-recently-used eviction
// policy is provided. Clients may use their own implementation if 
// they want something more sophisticated (like scan-resistance, a 
// custom eviction policy, variable cache sizing, etc.) 
//
#ifndef STORAGE_LEVELDB_INCLUDE_CACHE_H_
#define STORAGE_LEVELDB_INCLUDE_CACHE_H_

#include <stdint.h>
#include "leveldb/export.h"
#include "leveldb/slice.h"

namespace leveldb {

class LEVELDB_EXPORT Cache;

// Create a new cache with a fixed size capacity. This implementation
// of Cache uses a least-recently-used eviction policy.
LEVELDB_EXPORT Cache* NewLRUCache(size_t capacity);

class LEVELDB_EXPORT Cache {
public:
    Cache() { }

    // Destroys all existing entries by calling the "deleter"
    // function that was passed to the constructor.
    virtual ~Cache();

private:
    void LRU_Remove(Handle* e);



    // No copying allowed
    Cache(const Cache&);
    void operator=(const Cache&);
};  // class Cache

}   // namespace leveldb

#endif // STORAGE_LEVELDB_INCLUDE_CACHE_H_ 
