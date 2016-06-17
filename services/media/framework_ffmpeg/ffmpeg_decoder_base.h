// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_FFMPEG_FFMPEG_DECODER_BASE_H_
#define SERVICES_MEDIA_FRAMEWORK_FFMPEG_FFMPEG_DECODER_BASE_H_

#include "services/media/framework/parts/decoder.h"
#include "services/media/framework_ffmpeg/av_codec_context.h"
#include "services/media/framework_ffmpeg/av_frame.h"
#include "services/media/framework_ffmpeg/av_packet.h"
extern "C" {
#include "third_party/ffmpeg/libavcodec/avcodec.h"
}

namespace mojo {
namespace media {

// Abstract base class for ffmpeg-based decoders.
class FfmpegDecoderBase : public Decoder {
 public:
  FfmpegDecoderBase(AvCodecContextPtr av_codec_context);

  ~FfmpegDecoderBase() override;

  // Decoder implementation.
  std::unique_ptr<StreamType> output_stream_type() override;

  // Transform implementation.
  void Flush() override;

  bool TransformPacket(const PacketPtr& input,
                       bool new_input,
                       PayloadAllocator* allocator,
                       PacketPtr* output) override;

 protected:
  // Used to control deallocation of buffers.
  class AvBufferContext {
   public:
    AvBufferContext(size_t size, PayloadAllocator* allocator)
        : size_(size), allocator_(allocator) {
      DCHECK(allocator_);
      if (size_ == 0) {
        buffer_ = nullptr;
      } else {
        buffer_ =
            static_cast<uint8_t*>(allocator_->AllocatePayloadBuffer(size_));
      }
    }

    ~AvBufferContext() {
      if (allocator_ == nullptr) {
        // Previously released.
        return;
      }

      if (size_ != 0) {
        DCHECK(buffer_ != nullptr);
        allocator_->ReleasePayloadBuffer(size_, buffer_);
        return;
      }

      DCHECK(buffer_ == nullptr);
    }

    uint8_t* buffer() { return buffer_; }

    size_t size() { return size_; }

    // Releases ownership of the buffer.
    uint8_t* Release() {
      DCHECK(allocator_) << "AvBufferContext released twice";
      uint8_t* result = buffer_;
      buffer_ = nullptr;
      size_ = 0;
      allocator_ = nullptr;
      return result;
    }

   private:
    uint8_t* buffer_;
    size_t size_;
    PayloadAllocator* allocator_;
  };

  // Decodes from av_packet into av_frame_ptr. The result indicates how many
  // bytes were consumed from av_packet_. *frame_decoded_out indicates whether
  // av_frame_ptr contains a complete frame.
  virtual int Decode(const AVPacket& av_packet,
                     const ffmpeg::AvFramePtr& av_frame_ptr,
                     PayloadAllocator* allocator,
                     bool* frame_decoded_out) = 0;

  // Creates a Packet from av_frame.
  virtual PacketPtr CreateOutputPacket(const AVFrame& av_frame,
                                       PayloadAllocator* allocator) = 0;

  // Creates an end-of-stream packet with no payload.
  virtual PacketPtr CreateOutputEndOfStreamPacket() = 0;

  // The ffmpeg codec context.
  const AvCodecContextPtr& context() { return av_codec_context_; }

 private:
  // Prepares to process a new input packet.
  void PrepareInputPacket(const PacketPtr& input);

  // Finishes up after processing of an input packet has completed, possibly
  // producing a zero-size end-of-stream packet. Returns true to indicate that
  // a new input packet is required.
  bool UnprepareInputPacket(const PacketPtr& input, PacketPtr* output);

  AvCodecContextPtr av_codec_context_;
  AVPacket av_packet_;
  ffmpeg::AvFramePtr av_frame_ptr_;
};

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_FFMPEG_FFMPEG_DECODER_BASE_H_
