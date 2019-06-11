// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include "leveldb/table_builder.h"

struct TableBuilder::Rep {
    Options         options;
    Options         index_block_options;
    WritableFile*   file;
    uint64_t        offset;
    Status          status;
    BlockBuilder    data_block;
    BlockBuilder    index_block;
    std::string     last_key;
    int64_t         num_entries;
    bool            closed;         // Either Finish() or Abandon() has been called
    FilterBlockBuilder* filter_block;

    // We do not emit the index entry for a block until we have seen the 
    // first key for the next data block. This allows us to use shorter 
    // keys in the index block. For example, consider a block boundary
    // between the keys "the quick brown fox" and "the who". We can use
    // "the r" as the key for the index block entry since it is >= all
    // entries in the first block and < all entries in subsequent 
    // blocks.
    //
    // Invariant: r->pending_index_entry is true only if data_block is empty.
    bool pending_index_entry;
    BlockHandle pending_handle;     // Handle to add to index block

    std::string compressed_output;

    Rep(const Options& options, WritableFile* f)
        : options(opt),
          index_block_options(opt),
          file(f),
          offset(0),
          data_block(&options),
          index_block(&index_block_options),
          num_entries(0),
          closed(false),
          filter_block(opt.filter_policy == NULL ? NULL
                       : new FilterBlockBuilder(opt.filter_policy)),
          pending_index_entry(false) {
        index_block_options.block_restart_interval = 1;
    }
};  // struct Rep

TableBuilder::TableBuilder(const Options& options, WritableFile* file) 
    :   rep_(new Req(options, file)) {
    if (rep_->filter_block != NULL) {
        rep_->filter_block->StartBlock(0);
    }
}

TableBuilder::~TableBuilder() {
    assert(rep_->closed);   // Catch errors where callers forgot to call Finish()
    delete rep_->filter_block;
    delete rep_;
}



TableBuilder::Add(const Slice& key, const Slice& value) {
    Rep* r = rep_;
    assert(!r->closed);
    if (!ok()) return;
    if (r->num_entries > 0) {
        assert(r->options.comparator->Compare(key, Slice(r->last_key)) > 0);
    } 

    if (r->pending_index_entry) {
        assert(r->data_block.empty());
        r->options.comparator->FindShortestSeparator(&r->last_key, key);
        std::string handle_encoding;
        r->pending_handle.EncodeTo(&handle_encoding);
        r->index_block.Add(r->last_key, Slice(handle_encoding));
        r->pending_index_entry = false;
    }







}

