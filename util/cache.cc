// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include <assert.h>
#include "port/port.h"
#include "util/hash.h"
#include "util/mutexlock.h"

namespace leveldb {

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

