// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.
//
// A filter block is stored near the end of a Table file. It contains
// filters (e.g., bloom filters) for all data blocks in the table combined
// into a single filter block.

#ifndef STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
#define STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "leveldb/slice.h"
#include "util/hash.h"

namespace leveldb {

class FilterPolicy;

// A FilterBlockBuilder is used to construct all of the filters for a 
// particular Table. It generates a single string which is stored as 
// a special block in the Table.
//
// The sequence of calls to FilterBlockBuilder must match the regexp:
//      (StartBlock AddKey*)* Finish
//  ** to-catch
class FilterBlockBuilder {
public:
    explicit FilterBlockBuilder(const FilterPolicy*);

    void StartBlock(uint64_t block_offset);
    void AddKey(const Slice& key);
    Slice Finish();

private:
    void GenerateFilter();

    const FilterPolicy* policy_;
    std::string keys_;              // Flattened key contents
    std::vector<size_t> start_;     // Starting index in keys_ of each key


};  // class FilterBlockBuilder


}   // namespace leveldb

#endif  //  STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
