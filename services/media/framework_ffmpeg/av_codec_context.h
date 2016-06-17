// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_FFMPEG_AV_CODEC_CONTEXT_H_
#define SERVICES_MEDIA_FRAMEWORK_FFMPEG_AV_CODEC_CONTEXT_H_

#include "services/media/framework/types/stream_type.h"
#include "services/media/framework/types/video_stream_type.h"
extern "C" {
#include "third_party/ffmpeg/libavformat/avformat.h"
}

// Ffmeg defines this...undefine.
#undef PixelFormat

namespace mojo {
namespace media {

struct AVCodecContextDeleter {
  void operator()(AVCodecContext* context) const {
    avcodec_free_context(&context);
  }
};

using AvCodecContextPtr =
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;

struct AvCodecContext {
  static AvCodecContextPtr Create(const StreamType& stream_type);

  static std::unique_ptr<StreamType> GetStreamType(const AVCodecContext& from);
};

// Converts an AVPixelFormat to a PixelFormat.
VideoStreamType::PixelFormat PixelFormatFromAVPixelFormat(
    AVPixelFormat av_pixel_format);

// Converts a PixelFormat to an AVPixelFormat.
AVPixelFormat AVPixelFormatFromPixelFormat(
    VideoStreamType::PixelFormat pixel_format);

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_FFMPEG_AV_CODEC_CONTEXT_H_
