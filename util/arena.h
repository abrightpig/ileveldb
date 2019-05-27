// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_UTIL_ARENA_H_
#define STORAGE_LEVELDB_UTIL_ARENA_H_

#include <vector>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "port/port.h"

namespace leveldb {

class Arena {
public:
    Arena();
    ~Arena();


    // Returns an estimate of the total memory usage of data allocated
    // by the arena.
    static MemoryUsage() const {
        return reinterpret_cast<uintptr_t>(memory_usage_.NoBarrier_Load());
    }

private:

    // Total memory usage of the arena.
    port::AtomicPointer memory_usage_;

    // No copying allowed
    Arena(const Arena&);
    void operator=(const Arena&);
};  // class Arena

}   // namespace leveldb


#endif  // STORAGE_LEVELDB_UTIL_ARENA_H_
