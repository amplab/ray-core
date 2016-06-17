// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_PARTS_READER_CACHE_H_
#define SERVICES_MEDIA_FRAMEWORK_PARTS_READER_CACHE_H_

#include <atomic>
#include <map>
#include <vector>

#include "base/synchronization/lock.h"
#include "services/media/framework/parts/reader.h"
#include "services/media/framework/parts/sparse_byte_buffer.h"
#include "services/util/cpp/incident.h"

namespace mojo {
namespace media {

// Store for reading.
//
// ReaderCache is an Reader filter that reads an entire asset from an
// Reader into memory and implements Reader against the cache.
// Currently, there is no support for throttling the intake rate or for
// limiting the amount of memory used by the cache. The entire asset is read
// into memory and remains there until the cache is deleted.
// TODO(dalesat): Devise and implement a management policy.
//
// ReaderCache is implemented using a collection of holes (spans of the asset
// that haven't been read) and regions (spans of the asset that have been read).
// Holes can be indefinitely large, and no two holes are adjacent to each other.
// Regions represent successful past reads and can be any non-zero size.
//
// The intake side of ReaderCache chooses a hole to work on, reading regions
// and shrinking the hole from front to back. If the outlet side (the
// ReadAt implementation) needs content from a different part of the asset,
// the intake side finds the hole that starts at that position or creates one
// (by splitting an existing hole) and starts working on that. Once a hole is
// completely filled, intake moves to the next hole in order, wrapping around
// at the end of the asset. Once the entire asset is read, the intake side
// shuts down.
// TODO(dalesat): Provide methods for discovering what parts of the asset are
// cached.
class ReaderCache : public Reader {
 public:
  static std::shared_ptr<ReaderCache> Create(
      std::shared_ptr<Reader> upstream_reader);

  ~ReaderCache() override;

  // Reader implementation.
  void Describe(const DescribeCallback& callback) override;

  void ReadAt(size_t position,
              uint8_t* buffer,
              size_t bytes_to_read,
              const ReadAtCallback& callback) override;

 private:
  static constexpr size_t kDefaultReadSize = 32 * 1024;

  // Represents a pending ReadAt call. The buffer associated with the request
  // can be filled in sequential fragments using the CopyFrom method.
  class ReadAtRequest {
   public:
    ReadAtRequest();

    ~ReadAtRequest();

    // Initializes the request.
    void Start(size_t position,
               uint8_t* buffer,
               size_t bytes_to_read,
               const ReadAtCallback& callback);

    // Gets the current read position.
    size_t position() { return position_; }

    // Gets the remaining number of bytes to read.
    size_t remaining_bytes_to_read() { return remaining_bytes_to_read_; }

    // Delivers all or part of the data indicated by position and
    // remaining_bytes_to_read.
    void CopyFrom(uint8_t* source, size_t byte_count);

    // Completes the request with the indicated result.
    void Complete(Result result);

   private:
    std::atomic_bool in_progress_;
    size_t position_;  // Updated by CopyFrom.
    uint8_t* buffer_;  // Updated by CopyFrom.
    size_t original_bytes_to_read_;
    size_t remaining_bytes_to_read_;  // Updated by CopyFrom.
    ReadAtCallback callback_;
  };

  // Maintains the cached data in an in-memory data structure. Handles
  // fulfillment of at most one ReadAtRequest. Interacts with Intake to arrange
  // for the acquisition of data from the upstream reader. By default, intake
  // proceeds from the start of the asset to the end. The store deviates from
  // this order when a ReadAtRequest is submitted for data that isn't yet
  // cached. In that case, intake skips to the position of the ReadAtRequest
  // and proceeds from there, skipping filled regions and wrapping around as
  // needed until the entire asset is cached.
  class Store {
   public:
    Store();

    ~Store();

    // Initializes the store.
    void Initialize(Result result, size_t size, bool can_seek);

    // Calls the callback immediately with description values.
    void Describe(const DescribeCallback& callback);

    // Sets a read request to fulfill.
    void SetReadAtRequest(ReadAtRequest* request);

    // Determines what data intake should produce next. Returns kUnknownSize if
    // intake isn't required.
    size_t GetIntakePositionAndSize(size_t* size_out);

    // Submits intaken data.
    void PutIntakeBuffer(size_t position, std::vector<uint8_t>&& buffer);

    // Reports an intake error.
    void ReportIntakeError(Result result);

   private:
    // Attempts to progress satisfaction of the current read request.
    void ServeRequest();

    // These fields are stable after Initialize.
    size_t size_ = kUnknownSize;
    bool can_seek_ = false;

    mutable base::Lock lock_;
    Result result_ = Result::kOk;
    SparseByteBuffer sparse_byte_buffer_;
    SparseByteBuffer::Hole intake_hole_;
    SparseByteBuffer::Hole read_hole_;
    SparseByteBuffer::Region read_region_;
    ReadAtRequest* read_request_ = nullptr;
    size_t read_request_position_;
    size_t read_request_remaining_bytes_;
  };

  // Reads from the upstream reader into the store.
  class Intake {
   public:
    Intake();

    ~Intake();

    void Start(Store* store, std::shared_ptr<Reader> upstream_reader);

   private:
    void Continue();

    Store* store_;
    std::shared_ptr<Reader> upstream_reader_;
    std::vector<uint8_t> buffer_;
  };

  ReaderCache(std::shared_ptr<Reader> upstream_reader);

  ReadAtRequest read_at_request_;
  Store store_;
  Intake intake_;

  ThreadsafeIncident describe_is_complete_;
};

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_PARTS_READER_CACHE_H_
