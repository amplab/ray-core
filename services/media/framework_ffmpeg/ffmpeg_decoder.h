// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_FFMPEG_FFMPEG_DECODER_H_
#define SERVICES_MEDIA_FRAMEWORK_FFMPEG_FFMPEG_DECODER_H_

#include <memory>

#include "services/media/framework/parts/decoder.h"

namespace mojo {
namespace media {

// Abstract base class for ffmpeg-based decoders, just the create function.
// We don't want the base class implementation here, because we don't want
// dependent targets to have to deal with ffmpeg includes.
class FfmpegDecoder : public Decoder {
 public:
  // Creates an ffmpeg-based Decoder object for a given media type.
  static Result Create(const StreamType& stream_type,
                       std::shared_ptr<Decoder>* decoder_out);
};

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_FFMPEG_FFMPEG_DECODER_H_
