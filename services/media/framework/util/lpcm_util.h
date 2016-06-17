// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_UTIL_LPCM_UTIL_H_
#define SERVICES_MEDIA_FRAMEWORK_UTIL_LPCM_UTIL_H_

#include <memory>

#include "services/media/framework/types/audio_stream_type.h"

namespace mojo {
namespace media {

// Helper class that performs various LPCM processing functions.
class LpcmUtil {
 public:
  static std::unique_ptr<LpcmUtil> Create(const AudioStreamType& stream_type);

  virtual ~LpcmUtil() {}

  // Fills the buffer with silence.
  virtual void Silence(void* buffer, size_t frame_count) const = 0;

  // Copies samples.
  virtual void Copy(const void* in, void* out, size_t frame_count) const = 0;

  // Mixes samples.
  virtual void Mix(const void* in, void* out, size_t frame_count) const = 0;

  // Interleaves non-interleaved samples. This assumes ffmpeg non-interleaved
  // ("planar") layout, in which the buffer (in) is divided evenly into a
  // channel buffer per channel. The samples for each channel are contiguous
  // in the respective channel buffer with possible empty space at the end
  // (hence the in_type_count and the frame_count).
  virtual void Interleave(const void* in,
                          size_t in_byte_count,
                          void* out,
                          size_t frame_count) const = 0;
};

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_UTIL_LPCM_UTIL_H_
