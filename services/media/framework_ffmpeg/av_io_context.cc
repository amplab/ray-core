// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>

#include "base/logging.h"
#include "services/media/framework/parts/reader.h"
#include "services/media/framework_ffmpeg/av_io_context.h"
#include "services/media/framework_ffmpeg/ffmpeg_init.h"
extern "C" {
#include "third_party/ffmpeg/libavformat/avio.h"
}

namespace mojo {
namespace media {

void AVIOContextDeleter::operator()(AVIOContext* context) const {
  AvIoContextOpaque* av_io_context =
      reinterpret_cast<AvIoContextOpaque*>(context->opaque);
  DCHECK(av_io_context);
  delete av_io_context;
  av_free(context->buffer);
  av_free(context);
}

// static
AvIoContextPtr AvIoContext::Create(std::shared_ptr<Reader> reader) {
  // Internal buffer size used by AVIO for reading.
  constexpr int kBufferSize = 32 * 1024;

  InitFfmpeg();

  AvIoContextOpaque* avIoContextOpaque = new AvIoContextOpaque(reader);

  AVIOContext* avIoContext = avio_alloc_context(
      static_cast<unsigned char*>(av_malloc(kBufferSize)), kBufferSize,
      0,  // write_flag
      avIoContextOpaque, &AvIoContextOpaque::Read, nullptr,
      &AvIoContextOpaque::Seek);

  // Ensure FFmpeg only tries to seek when we know how.
  avIoContext->seekable =
      avIoContextOpaque->can_seek() ? AVIO_SEEKABLE_NORMAL : 0;

  // Ensure writing is disabled.
  avIoContext->write_flag = 0;

  return AvIoContextPtr(avIoContext);
}

// static
int AvIoContextOpaque::Read(void* opaque, uint8_t* buf, int buf_size) {
  AvIoContextOpaque* av_io_context =
      reinterpret_cast<AvIoContextOpaque*>(opaque);
  return av_io_context->Read(buf, buf_size);
}

// static
int64_t AvIoContextOpaque::Seek(void* opaque, int64_t offset, int whence) {
  AvIoContextOpaque* av_io_context =
      reinterpret_cast<AvIoContextOpaque*>(opaque);
  return av_io_context->Seek(offset, whence);
}

AvIoContextOpaque::~AvIoContextOpaque() {}

AvIoContextOpaque::AvIoContextOpaque(std::shared_ptr<Reader> reader)
    : reader_(reader) {
  reader->Describe([this](Result result, size_t size, bool can_seek) {
    describe_result_ = result;
    size_ = size == Reader::kUnknownSize ? -1 : static_cast<int64_t>(size);
    can_seek_ = can_seek;
    CallbackComplete();
  });

  WaitForCallback();
}

int AvIoContextOpaque::Read(uint8_t* buffer, size_t bytes_to_read) {
  DCHECK(position_ >= 0);

  if (position_ >= size_) {
    return 0;
  }

  DCHECK(static_cast<uint64_t>(position_) < std::numeric_limits<size_t>::max());

  Result read_at_result;
  size_t read_at_bytes_read;
  reader_->ReadAt(static_cast<size_t>(position_), buffer, bytes_to_read,
                  [this, &read_at_result, &read_at_bytes_read](
                      Result result, size_t bytes_read) {
                    read_at_result = result;
                    read_at_bytes_read = bytes_read;
                    CallbackComplete();
                  });

  WaitForCallback();

  if (read_at_result != Result::kOk) {
    LOG(ERROR) << "read failed";
    return AVERROR(EIO);
  }

  position_ += read_at_bytes_read;
  return read_at_bytes_read;
}

int64_t AvIoContextOpaque::Seek(int64_t offset, int whence) {
  switch (whence) {
    case SEEK_SET:
      position_ = offset;
      break;
    case SEEK_CUR:
      position_ += offset;
      break;
    case SEEK_END:
      if (size_ == -1) {
        LOG(WARNING) << "whence of SEEK_END, size unknown";
        return AVERROR(EIO);
      }
      position_ = size_ + offset;
      break;
    case AVSEEK_SIZE:
      if (size_ == -1) {
        LOG(WARNING) << "whence of AVSEEK_SIZE, size unknown";
        return AVERROR(EIO);
      }
      return size_;
    default:
      LOG(ERROR) << "unrecognized whence value " << whence;
      return AVERROR(EIO);
  }

  CHECK(size_ == -1 || position_ < size_) << "position out of range";
  return position_;
}

}  // namespace media
}  // namespace mojo
