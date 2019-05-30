// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include "util/arena.h"
#include <assert.h>

namespace leveldb {

static const int kBlockSize = 4096;

Arena::Arena() : memory_usage_(0) {
    alloc_ptr_ = NULL;      // First allocation will allocate a block
    alloc_bytes_remaining_ = 0;
}

Arena::~Arena() {
    for (size_t i = 0; i < blocks_.size(); i++) {
        delete[] blocks_[i];
    }
}

char* Arena::AllocateFallback(size_t bytes) {
    if (bytes > kBlockSize / 4) {
        // Object is more than a quarter of our block size. Allocate it seperately
        // to avoid wasting too much space in leftover bytes
        char* result = AllocatedNewBlock(bytes);
        return result; 
    }

    // We waste the remaining space in the current block.
    alloc_ptr_ = AllocateNewBlock(kBlockSize);
    alloc_bytes_remaining_ = kBlockSize;

    char* result = alloc_ptr_;   
    alloc_ptr += bytes;
    alloc_byte_remaining_ -= bytes;
    return result;
}

char* Arena::AllocateNewBlock(size_t block_bytes) {
    char* result = new char[block_bytes];
    blocks_.push_back(result);
    memory_usage_.NoBarrier_Store(
            reinterpret_cast<void*>(MemoryUsage() + block_bytes + sizeof(char*)));
            // ** to-catch: why + sizeof(char*) ??
    return result;
}

}   // namespace leveldb
