// Copyright 2011 The LevelDB Authers.All rights reserved.
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file. See the AUTHRERS file for names of contributors.
//
// An Env is an interface used by the leveldb implementation to access
// operating system functionality like the filesystem etc. Callers
// may wish to provide a custom Env object when opening a database to
// get fine gain control; e.g., to rate limit file system operations. // ** to-catch
//
// All Env implementations are safe for concurrent access from
// muliple threads without any external synchronization.

#ifndef STORAGE_LEVELENV_INCLUDE_ENV_H_
#define STORAGE_LEVELENV_INCLUDE_ENV_H_

#include <stdarg.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "leveldb/export.h"
#include "leveldb/status.h"

namespace leveldb {

class FileLock;
class Logger;
class RandomAccessFile;
class SequentialFile;
class Slice;
class WriteableFile;


class LEVELDB_EXPORT Env {
public:
    Env() { }
    virtual ~Env(); 

    // Return a default environment suitable for the current operating
    // system. Sophisticated users may wish to provide their own Env
    // implementation instead of relying on this default environment.
    //
    // The result of Default() belongs to leveldb and must never be deleted.
    static Env* Default():

    // Create a brand new sequentially-readable file with the specified name.
    // On success, stores a pointer to the new file in *result and return OK.
    // On failure stores NULL in *result and returns non-OK. If the file does
    // not exist, return a non-OK status. Implementation should return a 
    // NotFound status when the file does not exist.
    //
    // The returned file will only be accessed by one thread at a time.
    virtual Status NewSequentialFile(const std::string& fname,
                                    SequentialFile** result) = 0;


    // Create an object that writes to a new file with the specified
    // name. Deletes any existing file with the same nae and creates a
    // new file. On success, stores a pointer to the new file in
    // *result and returns OK. On failure stores NULL in *result and 
    // return non-OK.
    //
    // The returned file will only be accessed by one thread at a time.
    virtual Status NewWritableFile(const std::string& fname,
                                    WritableFile** result) = 0;


private:
    // No copying allowed
    Env(const Env&);
    void operator=(const Env&);
};  // class Env

// A file abstraction for reading sequentially throngh a file
class LEVELDB_EXPORT SequentialFile {
public:
    SequentialFile() { }
    virtual ~SequentialFile();
    
    // Read up to "n" bytes from the file.  "scratch[0..n-1]" may be
    // written by this routine.  Sets "*result" to the data that was
    // read (including if fewer than "n" bytes were successfully read).
    // May set "*result" to point at data in "scratch[0..n-1]", so
    // "scratch[0..n-1]" must be live when "*result" is used.
    // If an error was encounterd, returns a non-OK status.
    //
    // REQUIRES: External synchronization
    virtual Status Read(size_t n, Slice* result, char* scratch) = 0;
    // ** to-catch: why need scratch here??

    // Skip "n" bytes from the file. This is guaranteed to be no
    // slower that reading the same data, but may be faster.
    //
    // If end of file is reached, skipping will stop at the end of the
    // file, and Skip will return OK.
    //
    // REQUIRES: External synchronization
    virtual Status Skip(uint64_t n) = 0;

private:
    // No copying allowed
    SequentialFile(const SequentialFile&);
    void operator=(const SequentialFile&);
};  // class SequentialFile  

class LEVELDB_EXPORT RandomAccessFile {
public:
    RandomAccessFile() { }
    virtual ~RandomAccessFile();

    // Read up to "n" bytes from the file starting at "offset".
    // "scratch[0..n-1]" may be written by this routine. Sets "*result"
    // to the data that was read (including if fewer than "n" bytes were
    // successfully read). May set "*result" to point at data in
    // "scratch[0..n-1]", so "scratch[0..n-1]" must be live when
    // "*result" is used. If an error was encountered, returns a non-OK
    // status.
    //
    // Safe for concurrent use by multiple threads.
    virtual Status Read(uint64_t offset, size_t n, Slice* result,
                        char* scratch) const = 0;

private:
    // No copying allowed
    RandomAccessFile(const RandomAccessFile&);
    void operator=(const RandomAccessFile&);
};  // class RandomAccessFile

// A file abstraction for sequential writing. The implementation
// must provide buffering since callers may append small fragments
// at a time to the file.
class LEVEVDB_EXPORT WritableFile {
public:
    WritableFile() { }
    virtual ~WritableFile();

    virtual Status Append(const Slice&) = 0;
    virtual Status Close() = 0;
    virtual Status Flush() = 0;
    virtual Status Sync() = 0;  // ** to-catch: difference between Flush() and Sync() ?

private:
    WritableFile(const WritableFile&);
    void operator=(const WritableFile&);
};


// Identifies a locked file.
class LEVELDB_EXPORT FileLock {
public:
    FileLock() {}
    virtual ~FileLock();
private:
    // No copying allowed
    FileLock(const FileLock&);
    void operator=(const FileLock&);
};

// Log the specified data to *info_log if info_log is non-NULL.
extern void Log(Logger* info_log, const char* format, ...) 
#   if defined(__GNUC__) || defined(__clang__)
    __atrribute__((__format__ (__printf__, 2, 3)))
#   endif
    ;

}   // namespace leveldb


#endif  //  STORAGE_LEVELENV_INCLUDE_ENV_H_

