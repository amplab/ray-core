// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "base/logging.h"
#include "services/media/framework_ffmpeg/ffmpeg_formatting.h"
#include "services/media/framework_ffmpeg/ffmpeg_video_decoder.h"
extern "C" {
#include "third_party/ffmpeg/libavutil/imgutils.h"
}

namespace mojo {
namespace media {

FfmpegVideoDecoder::FfmpegVideoDecoder(AvCodecContextPtr av_codec_context)
    : FfmpegDecoderBase(std::move(av_codec_context)) {
  DCHECK(context());

  context()->opaque = this;
  context()->get_buffer2 = AllocateBufferForAvFrame;
  context()->refcounted_frames = 1;
}

FfmpegVideoDecoder::~FfmpegVideoDecoder() {}

int FfmpegVideoDecoder::Decode(const AVPacket& av_packet,
                               const ffmpeg::AvFramePtr& av_frame_ptr,
                               PayloadAllocator* allocator,
                               bool* frame_decoded_out) {
  DCHECK(allocator);
  DCHECK(frame_decoded_out);
  DCHECK(context());
  DCHECK(av_frame_ptr);

  DCHECK(av_packet.pts != AV_NOPTS_VALUE);

  // Use the provided allocator (for allocations in AllocateBufferForAvFrame).
  allocator_ = allocator;

  // We put the pts here so it can be recovered later in CreateOutputPacket.
  // Ffmpeg deals with the frame ordering issues.
  context()->reordered_opaque = av_packet.pts;

  int frame_decoded = 0;
  int input_bytes_used = avcodec_decode_video2(
      context().get(), av_frame_ptr.get(), &frame_decoded, &av_packet);
  *frame_decoded_out = frame_decoded != 0;

  // We're done with this allocator.
  allocator_ = nullptr;

  return input_bytes_used;
}

void FfmpegVideoDecoder::Flush() {
  FfmpegDecoderBase::Flush();
  next_pts_ = Packet::kUnknownPts;
}

PacketPtr FfmpegVideoDecoder::CreateOutputPacket(const AVFrame& av_frame,
                                                 PayloadAllocator* allocator) {
  DCHECK(allocator);

  // Recover the pts deposited in Decode.
  next_pts_ = av_frame.reordered_opaque;

  AvBufferContext* av_buffer_context =
      reinterpret_cast<AvBufferContext*>(av_buffer_get_opaque(av_frame.buf[0]));

  return Packet::Create(
      next_pts_,
      false,  // The base class is responsible for end-of-stream.
      av_buffer_context->size(), av_buffer_context->Release(), allocator);
}

PacketPtr FfmpegVideoDecoder::CreateOutputEndOfStreamPacket() {
  return Packet::CreateEndOfStream(next_pts_);
}

int FfmpegVideoDecoder::AllocateBufferForAvFrame(
    AVCodecContext* av_codec_context,
    AVFrame* av_frame,
    int flags) {
  // It's important to use av_codec_context here rather than context(),
  // because av_codec_context is different for different threads when we're
  // decoding on multiple threads. Be sure to avoid using self->context().

  // CODEC_CAP_DR1 is required in order to do allocation this way.
  DCHECK(av_codec_context->codec->capabilities & CODEC_CAP_DR1);

  FfmpegVideoDecoder* self =
      reinterpret_cast<FfmpegVideoDecoder*>(av_codec_context->opaque);
  DCHECK(self);
  DCHECK(self->allocator_);

  Extent visible_size(av_codec_context->width, av_codec_context->height);
  const int result =
      av_image_check_size(visible_size.width(), visible_size.height(), 0, NULL);
  if (result < 0) {
    return result;
  }

  // FFmpeg has specific requirements on the allocation size of the frame.  The
  // following logic replicates FFmpeg's allocation strategy to ensure buffers
  // are not overread / overwritten.  See ff_init_buffer_info() for details.

  // When lowres is non-zero, dimensions should be divided by 2^(lowres), but
  // since we don't use this, just DCHECK that it's zero.
  DCHECK_EQ(av_codec_context->lowres, 0);
  Extent coded_size(
      std::max(visible_size.width(),
               static_cast<size_t>(av_codec_context->coded_width)),
      std::max(visible_size.height(),
               static_cast<size_t>(av_codec_context->coded_height)));

  VideoStreamType::FrameLayout frame_layout;

  VideoStreamType::InfoForPixelFormat(
      PixelFormatFromAVPixelFormat(av_codec_context->pix_fmt))
      .BuildFrameLayout(coded_size, &frame_layout);

  AvBufferContext* av_buffer_context =
      new AvBufferContext(frame_layout.size, self->allocator_);
  uint8_t* buffer = av_buffer_context->buffer();

  // TODO(dalesat): For investigation purposes only...remove one day.
  if (self->first_frame_) {
    self->first_frame_ = false;
    self->colorspace_ = av_codec_context->colorspace;
    self->coded_size_ = coded_size;
  } else {
    if (av_codec_context->colorspace != self->colorspace_) {
      LOG(WARNING) << " colorspace changed to " << av_codec_context->colorspace
                   << std::endl;
    }
    if (coded_size.width() != self->coded_size_.width()) {
      LOG(WARNING) << " coded_size width changed to " << coded_size.width()
                   << std::endl;
    }
    if (coded_size.height() != self->coded_size_.height()) {
      LOG(WARNING) << " coded_size height changed to " << coded_size.height()
                   << std::endl;
    }
    self->colorspace_ = av_codec_context->colorspace;
    self->coded_size_ = coded_size;
  }

  if (buffer == nullptr) {
    LOG(ERROR) << "failed to allocate buffer of size " << frame_layout.size;
    return -1;
  }

  // Decoders require a zeroed buffer.
  std::memset(buffer, 0, frame_layout.size);

  for (size_t plane = 0; plane < frame_layout.plane_count; ++plane) {
    av_frame->data[plane] = buffer + frame_layout.plane_offset_for_plane(plane);
    av_frame->linesize[plane] = frame_layout.line_stride_for_plane(plane);
  }

  // TODO(dalesat): Do we need to attach colorspace info to the packet?

  av_frame->width = coded_size.width();
  av_frame->height = coded_size.height();
  av_frame->format = av_codec_context->pix_fmt;
  av_frame->reordered_opaque = av_codec_context->reordered_opaque;

  DCHECK(av_frame->data[0] == buffer);
  av_frame->buf[0] =
      av_buffer_create(buffer, av_buffer_context->size(),
                       ReleaseBufferForAvFrame, av_buffer_context,
                       0);  // flags

  return 0;
}

void FfmpegVideoDecoder::ReleaseBufferForAvFrame(void* opaque,
                                                 uint8_t* buffer) {
  AvBufferContext* av_buffer_context =
      reinterpret_cast<AvBufferContext*>(opaque);
  DCHECK(av_buffer_context);
  // Either this buffer has already been released to someone else's ownership,
  // or it's the same as the buffer parameter.
  DCHECK(av_buffer_context->buffer() == nullptr ||
         av_buffer_context->buffer() == buffer);
  delete av_buffer_context;
}

}  // namespace media
}  // namespace mojo
