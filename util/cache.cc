// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "leveldb/cache.h"
#include "port/port.h"
#include "util/hash.h"
#include "util/mutexlock.h"

namespace leveldb {

// LRU cache implementation
//
// Cache entries have an "in_cache" boolean indicating whether the cache has a
// reference on the entry. The only ways that this can become false without the
// entry being passed to its "deleter" are via Erase(), via Insert() when 
// an element with a duplicate key is inserted, or on destruction of the cache.
//
// The cache keeps two linked lists of items in the cache. All items in the
// cache are in one list or the other, and never both. Items still referenced
// by clients but erased from the cache are in neither list. The lists are:
// - in-use:    contains the items currently referenced by clients, in no 
//   particular order. (This list is used for invariant checking. If we
//   removed the check, elements that would otherwise be on this list could be
//   left as disconnected singleton lists.) ** to-catch
// - LRU:   contains the items not currently referenced by clients, in LRU order
// Elements are moved between these lists by the Ref() and Unref() methods,
// when they detect an element in the cache acquiring or losing its only
// external reference.
//
// An entry is a variable length heap-allocated structure. Entries
// are kept in a circular doubly linked list ordered by access time.
struct LRUHandle {
    void* value;
    void (*deleter)(const Slice&, void* value);
    LRUHandle* next_hash;
    LRUHandle* next;
    LRUHandle* prev;
    size_t charge;          // TODO(opt): Only allow uint32_t?
    size_t key_length;
    bool in_cache;          // Whether entry is in the cache.
    uint32_t refs;          // References, including cache reference, if present.
    uint32_t hash;          // Hash of key(); used for fast sharding and comparisons
    char key_data[1];       // Beginning of key

    Slice key() const {
        // next_ is only equal to this if the LRU handle is the list head of an
        // empty list. List heads never have meaningful keys.
        assert(next != this);

        return Slice(key_data, key_length);
    }
};

Cache::~Cache() {

}

namespace {

// A single shard of sharded cache.
class LRUCache {
public:
    LRUCache();
    ~LRUCache();

private:
    

};  // class LRUCache

class ShardedLRUCache : public Cache {
private:
    LRUCache shard_[kNumShards];
    port::Mutex id_mutex_;
    uint64_t last_id_;

public:

};  // class ShardedLRUCache

}   // end anonymous namespace

Cache* NewLRUCache(size_t capacity) {
    return new SharedLRUCache(capacity);
}


}   // namespace leveldb

