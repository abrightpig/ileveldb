// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include "leveldb/table.h"

#include "leveldb/cache.h"
#include "leveldb/comparator.h"
#include "leveldb/env.h"
#include "leveldb/filter_policy.h"
#include "leveldb/options.h"
#include "table/block.h"
#include "table/filter_block.h"
#include "table/format.h"
#include "table/two_level_iterator.h"
#include "util/coding.h"

namespace leveldb {

struct Table::Rep {
    ~Rep() {
        delete filter;
        delete [] fiter_data;
        delete index_block;
    }

    Options options;
    Status status;
    RandomAccessFile* file;
    uint64_t cache_id;
    FilterBlockReader* filter;
    const char* filter_data;

    BlockHandle metaindex_handle;   // Handle to metaindex_block: save from footer
    Block* index_block;
};

Status Table::Open(const Options& options,
                   RandomAccessFile* file,
                   uint64_t size,
                   Table** table) {
    *table = NULL;
    if (size < Footer::kEncodedLength) {
        return Status::Corruption("file is too short to be an sstable");
    }

    char footer_space[Footer::kEncodedLength];
    Slice footer_input;
    Status s = file->Read(size - Footer::kEncodedLength, Footer::kEncodedLength,
                          &footer_input, footer_space);
    if (!s.ok()) return s;

    Footer footer;
    s = footer.DecodeFrom(&footer_input);
    if (!s.ok()) return s;

    // Read the index block
    BlockContents index_block_contents;
    if (s.ok()) {
        ReadOptions opt;
        if (options.paranoid_checks) {
            opt.verify_checksums = true;
        }
        s = ReadBlock(file, opt, footer.index_handle(), &index_block_contents);
    }

    if (s.ok()) {
        // We've successfully read the footer and the index block: we're
        // ready to serve requests.
        Block* index_block = new Block(index_block_contents);
    }
    //***********************************
    //***********************************
    //***********************************
    //***********************************
}

// Convert an index iterator value (i.e., an encoded BlockHandle)
// into an iterator over the contents of the corresponding block.
Iterator* Table::BlockReader(void* arg,
                             const ReadOptions& options,
                             const Slice& index_value) {
    Table* table = reinterpret_cast<Table*>(arg);
    Cache* block_cache = table->rep_->options.block_cache;

}

Iterator* Table::NewIterator() const {
    return NewTwoLevelIterator(
            rep_->index_block->NewIterator(rep_->options.comparator),
            &Table::BlockReader, const_cast<Table*>(this), options);
}


}   // namespace leveldb
