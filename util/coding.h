// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.
//
// Endian-neutral encoding:
// * Fixed-length numbers are encoded with least-significant byte first
// * In addtion we support variable length "varint" encoding
// * Strings are encoded prefixed by their length in variant format
//
#ifndef STORAGE_LEVELDB_UTIL_CODING_H_
#define STORAGE_LEVELDB_UTIL_CODING_H_

#include <stdio.h>
#include <string.h>
#include <string>
#include "leveldb/slice.h"
#include "port/port.h"

namespace leveldb {

// Standard Put... routines append to a string
extern void PutFixed32(std::string* std, uint32_t value);
extern void PutFixed64(std::string* std, uint64_t value);

// Standard Get... routines parse a value from the beginning of a Slice
// and advance the slice past the parsed value.
extern bool GetLengthPrefixedSlice(Slice* input, Slice* result);

// Lower-level versions of Put... that write directly into a character buffer
// REQUIRES: dst has enough space for the value being written
extern void EncodeFixed32(char* dst, uint32_t value);
extern void EncodeFixed64(char* dst, uint64_t value);


inline uint32_t DecodeFixed32(const char* ptr) {
    if (port::kLittleEndian) {
        // Load the raw bytes
        uint32_t result;
        memcpy(&result, ptr, sizeof(result));   // gcc optimizes this to a plain load
        return result;
    }
    else {
        return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0])))
            | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8)
            | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16)
            | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
    }
}

inline uint64_t DecodeFixed64(const char* ptr) {
    if (port::kLittleEndian) {
        // Load the raw bytes
        uint64_t result;
        memcpy(&result, ptr, sizeof(result));   // gcc optimizes this to a plain load
        return result;
    }
    else {
        uint64_t lo = DecodeFixed32(ptr);
        uint64_t hi = DecodeFixed32(ptr + 4);
        return (hi << 32) | lo;
    }
}

// Internal routine for use by fallback path of GetVarint32Ptr
extern const char* GetVarint32PtrFallback(const char* p,
                                          const char* limit, 
                                          uint32_t* value);

inline const char* GetVarint32Ptr(const char* p,
                                  const char* limit,
                                  uint32_t* value) {
    if (p < limit) {
        uint32_t result = *(reinterpret_cast<const unsigned char*>(p));
        if ((result & 128) == 0) {
            *value = result;
            return p + 1;
        }
    }
    return GetVarint32PtrFallback(p, limit, value);
}

}   // namespace leveldb

#endif  //
