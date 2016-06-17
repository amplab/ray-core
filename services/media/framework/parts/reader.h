// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_PARTS_READER_H_
#define SERVICES_MEDIA_FRAMEWORK_PARTS_READER_H_

#include <limits>
#include <memory>

#include "services/media/framework/result.h"

namespace mojo {
namespace media {

// Abstract base class for objects that read raw data on behalf of demuxes.
class Reader {
 public:
  using DescribeCallback =
      std::function<void(Result result, size_t size, bool can_seek)>;
  using ReadAtCallback = std::function<void(Result result, size_t bytes_read)>;

  static constexpr size_t kUnknownSize = std::numeric_limits<size_t>::max();

  virtual ~Reader() {}

  // Returns a result, the file size and whether the reader supports seeking
  // via a callback. The returned size is kUnknownSize if the content size isn't
  // known.
  virtual void Describe(const DescribeCallback& callback) = 0;

  // Reads the specified number of bytes into the buffer from the specified
  // position and returns a result and the number of bytes read via the
  // callback.
  virtual void ReadAt(size_t position,
                      uint8_t* buffer,
                      size_t bytes_to_read,
                      const ReadAtCallback& callback) = 0;
};

}  // namespace media
}  // namespace mojo

#endif  // MOJO_SERVICES_MEDIA_READER_H_
