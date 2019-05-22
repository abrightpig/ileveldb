// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.
//
// Thread-safe(provide internal synchronization)
// ** to-catch : how to accomplish this?

#ifndef STORAGE_LEVELDB_DB_DB_LOG_FORMAT_H_
#define STORAGE_LEVELDB_DB_DB_LOG_FORMAT_H_

namespace leveldb {
namespace log {

enum RecordType {
    // Zero is reserved for preallocated files
    kZeroType = 0,

    kFullType = 1,

    // For fragment
    kFirstType = 2,
    kMiddleType = 3,
    kLastType = 4
}
static const int kMaxRecordType = kLastType;

//** to-add


}
}   // namespace leveldb


#endif  //  STORAGE_LEVELDB_DB_DB_LOG_FORMAT_H_
