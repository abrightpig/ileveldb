// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include "table/two_level_iterator.h"

#include "leveldb/table.h"
#include "table/block.h"
#include "table/format.h"
#include "table/iterator_wrapper.h"

namespace leveldb {

namespace {

typedef Iterator* (*BlockFunction)(void*, const ReadOptions&, const Slice&); 

class TwoLevelIterator: public Iterator {
public:
    TwoLevelIterator(
        Iterator* index_iter,
        BlockFunction block_function,
        void* arg,
        const ReadOption& options); 

    virtual ~TwoLevelIterator();

    virtual void Seek(const Slice& target);
    virtual void SeekToFirst();
    virtual void SeekToLast();
    virtual void Next();
    virtual void Prev();

    virtual bool Valid() const {
        return data_iter_.Valid();
    }
    virtual Slice key() const {
        assert(Valid());
        return data_iter_.key();
    }
    virtual Slice value() const {
        assert(Valid());
        return data_iter_.value();
    }

pvivate:    
    
    BlockFunction block_function_;
    void* arg_;
    const ReadOptions options_;
    Status status_;
    IteratorWrapper index_iter_;
    IteratorWrapper data_iter_;     // May be NULL


};  // class Iterator

TwoLevelIterator::TwoLevelIterator(
    Iterator* index_iter,
    BlockFunction block_function,
    void* arg,
    const ReadOption& options)
    : block_function_(block_function),
      arg_(arg),
      options_(options),
      index_iter_(index_iter),
      data_iter_(NULL) {
}

TwoLevelIterator::~TwoLevelIterator() {
}

}   // namespace

Iterator* NewTwoLevelIterator(
        Iterator* index_iter,
        BlockFunction block_function,
        void* arg,
        const ReadOptions& options) {
    return new TwoLevelIterator(index_iter, block_function, arg, options);

}

}   // namespace leveldb
