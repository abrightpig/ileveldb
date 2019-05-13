// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#ifndef STORAGE_LEVELSTATUS_INCLUDE_STATUS_H_
#define STORAGE_LEVELSTATUS_INCLUDE_STATUS_H_

#include <string>
#include "leveldb/export.h"
#include "leveldb/slice.h"


namespace leveldb {

class LEVELDB_EXPORT Status {
public:
    // Create a success status.
    Status() : state_(NULL) { }
    ~Status() { delete[] state_; }  // **to-catch: why delete state_?

    // Copy the specified status
    Status(const Status& s);
    void operator=(const Status& s);

    // Return a success status.
    static Status OK() { return Status(); } 

    // Return error status of an appropriate type.
    statis Status NotFound(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kNotFound, msg, msg2);
    } 

    statis Status Corruption(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kCorruption, msg, msg2);
    } 

    statis Status NotSupported(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kNotSupported, msg, msg2);
    } 

    statis Status InvalidArgument(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kInvalidArgument, msg, msg2);
    } 

    statis Status IOError(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kIOError, msg, msg2);
    } 

    // Return true iff the status indicates success.
    bool ok() const { return (state_ == NULL); }

    // Return true iff the status indicates a NotFound error.
    bool IsNotFound() const { return code() == kNotFound; }

    // Return true iff the status indicates a Corruption error.
    bool IsCorruption() const { return code() == kCorruption; }

    // Return true iff the status indicates a IOError error.
    bool IsIOError() const { return code() == kIOError; }

    // Return true iff the status indicates a IsNotSupportedError error.
    bool IsIsNotSupportedError() const { return code() == kIsNotSupportedError; }

    // Return true iff the status indicates a IsInvalidArgument error.
    bool IsIsInvalidArgument() const { return code() == kIsInvalidArgument; }

    // Return a string representation of this status suitable for printing.
    // Returns the string "OK" for success.
    std::string ToString() const;

private:private:
    // OK status has a NULL state_.Otherwise, state_ is a new[] array.
    // of the following form:
    //      state_[0..3] == length of the message
    //      state_[4]    == code
    //      state_[5..]  == message
    const char* state_;
    
    enum Code {
        kOk = 0;
        kNotFound = 1,
        kCorruption = 2,
        kNotSupported = 3,
        kInvalidArgument = 4,
        kIOError = 5
    };

    Code code() const {
        return (state_ == NULL) ? kOk : static_cast<Code>(state_[4]);
    }

    Status(Code code, const Slice& msg, const Slice& msg2);
    static const char* CopyState(const char* s);
}

inline Status::Status(const Status& s) {
    state_ = (s.state_ == NULL) ? NULL : CopyState(s.state_);
}




} // leveldb


#endif  // STORAGE_LEVELSTATUS_INCLUDE_STATUS_H_
