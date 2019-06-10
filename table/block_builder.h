// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#ifndef STORAGE_LEVELDB_TABLE_BLOCK_BUILDER_H_
#define STORAGE_LEVELDB_TABLE_BLOCK_BUILDER_H_

#include <vector>

#include <stdint.h>
#include "leveldb/slice.h"

namespace leveldb {

struct Options;

class BlockBuilder {
public:
    explicit BlockBuilder(const Options* options);

private:
    const Options*          options_;
    std::string             buffer_;    // Destination buffer
    std::vector<uint32_t>   restart_;   // Restart points
    int                     counter_;   // Number of entries emitted since restart
    bool                    finished_;  // Has Finish() been called?
    std::string             last_key_;
    
    // No copying allowed
    BlockBuilder(const BlockBuilder&);
    void operator=(const BlockBuilder&);
};  // class BlockBuilder

}   // namespace leveldb

#endif  // STORAGE_LEVELDB_TABLE_BLOCK_BUILDER_H_
