// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/media/framework/parts/decoder.h"
#include "services/media/framework_ffmpeg/ffmpeg_decoder.h"

namespace mojo {
namespace media {

Result Decoder::Create(const StreamType& stream_type,
                       std::shared_ptr<Decoder>* decoder_out) {
  std::shared_ptr<Decoder> decoder;
  Result result = FfmpegDecoder::Create(stream_type, &decoder);
  if (result == Result::kOk) {
    *decoder_out = decoder;
  }

  return result;
}

}  // namespace media
}  // namespace mojo
