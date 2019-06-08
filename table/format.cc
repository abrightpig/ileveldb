// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.

#include "table/format.h"

#include "leveldb/env.h"
#include "port/port.h"
#include "table/block.h"
#include "util/coding.h"
#include "util/crc32c.h"

namespace leveldb {

Status BlockHandle::DecodeFrom(Slice* input) {
    if (GetVarint64(input, &offset_) && 
        GetVarint64(input, &size_)) {
        return Status::OK();
    }
    else {
        return Status::Corruption("bad block handle"); 
    }
}

Status Footer::DecodeFrom(Slice* input) {
    const char* magic_ptr = input->data() + kEncodedLength - 8;
    const uint32_t magic_lo = DecodeFixed32(magic_ptr);
    const uint32_t magic_hi = DecodeFixed32(magic_ptr + 4);
    const uint64_t magic = ((static_cast<uint64_t>(magic_hi) << 32) |
                            (static_cast<uint64_t>(magic_lo)));
    if (magic != kTableMagicNumber) {
        return Status::Corruption("not an sstable (bad magic number)"); 
    }

    Status result = metaindex_handle_.DecodeFrom(input);
    if (result.ok()) {
        result = index_handle_.DecodeFrom(input);
    }
    if (result.ok()) {
        // We skip over any leftover data (just padding for now) in "input"
        const char* end = magic_ptr + 8;
        *input = Slice(end, input->data() + input->size() - end);
    }
    return result;
}


}   // namespace leveldb
