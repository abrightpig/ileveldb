// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.
//
//
#ifndef STORAGE_LEVELDB_INCLUDE_ITERATOR_H_
#define STORAGE_LEVELDB_INCLUDE_ITERATOR_H_

#include "leveldb/export.h"
#include "leveldb/slice.h"
#include "leveldb/status.h"

namespace leveldb {

class LEVELDB_EXPORT Iterator {
public:
    Iterator();
    virtual ~Iterator();
    
    // An iterator is either positioned at a key/value pair, or 
    // not valid. This method returns true iff the iterator is valid.
    virtual bool Valid() const = 0;

    // Position at the first key in the source. The iterator is Valid()
    // after this call iff the source is not empty.
    virtual bool SeekToFirst() = 0;

    // Position at the last key in the source. Ther iterator is 
    // Valid() after this call iff the source is not empty.
    virtual bool SeekToLast() = 0;

    // Position at the first key in the source that is at or past target.
    // The iterator is Valid() after this call iff the source contains
    // an entry that comes at or past target.
    virtaul void Seek(const Slice& target) = 0;

    // Moves to the next entry in the source. After this call, Valid() is 
    // true iff the iterator was not positioned at the last entry in the source.
    // REQUIRES: Valid()
    virtual void Next() = 0;

    // Moves to the previous entry in the source. After this call, Valid() is 
    // true iff the iterator was not positioned at the first entry in source.
    // REQUIRES: Valid()
    virtual void Prev() = 0;

    // Return the key for the current entry. The underlying storage for 
    // the returned slice is valid only util the next modification of 
    // the iterator.
    // REQUIRES: Valid()
    virtual Slice key() const = 0;

    // Return the value for the current entry. The underlying storage for 
    // the returned slice is valid only util the next modification of 
    // the iterator.
    // REQUIRES: Valid()
    virtual Slice value() const = 0;

    // If an error has occurred, return it. Else return an ok status.
    virtual Status status() const = 0;

    // Note that unlike all of the preceding methods, this method is
    // not abstract and therefore clients should not override it.
    typedef void (*CleanupFunction)(void* arg1, void* arg2);
    void RegisterCleanup(CleanupFunction function, void* arg1, void* arg2);


private:
    struct Cleanup {
        CleanupFunction function;
        void* arg1;
        void* arg2;
        Cleanup* next;
    };
    Cleanup cleanup_;

    // No copying allowed
    Iterator(const Iterator&);
    void operate=(const Iterator&);
};  // class Iterator

// Return an empty iterator (yields nothing).
LEVELDB_EXPORT Iterator* NewEmptyIterator();

// Return an emtpy iterator with the specified status.
LEVELDB_EXPORT Iterator* NewErrorIterator(const Status& status);

}   // namespace leveldb

#endif  //  STORAGE_LEVELDB_INCLUDE_ITERATOR_H
