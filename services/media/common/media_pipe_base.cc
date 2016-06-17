// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "services/media/common/media_pipe_base.h"

namespace mojo {
namespace media {

MediaPipeBase::MediaPipeBase()
  : binding_(this) {
}

MediaPipeBase::~MediaPipeBase() {
}

MojoResult MediaPipeBase::Init(InterfaceRequest<MediaConsumer> request) {
  // Double init?
  if (IsInitialized()) {
    return MOJO_RESULT_ALREADY_EXISTS;
  }

  binding_.Bind(request.Pass());
  binding_.set_connection_error_handler([this]() -> void {
    Reset();
  });
  return MOJO_RESULT_OK;
}

bool MediaPipeBase::IsInitialized() const {
  return binding_.is_bound();
}

void MediaPipeBase::Reset() {
  if (binding_.is_bound()) {
    binding_.Close();
  }
  buffer_ = nullptr;
}

void MediaPipeBase::SetBuffer(ScopedSharedBufferHandle handle,
                              const SetBufferCallback& cbk) {
  DCHECK(handle.is_valid());

  // Double init?  Close the connection.
  if (buffer_) {
    LOG(ERROR) << "Attempting to set a new buffer on a MediaConsumer which "
                  "already has a buffer assigned. (size = "
               << buffer_->size()
               << ")";
    Reset();
    return;
  }

  // Query the buffer for its size.  If we fail to query the info, close the
  // connection.
  MojoResult res;
  MojoBufferInformation info;
  res = MojoGetBufferInformation(handle.get().value(), &info, sizeof(info));
  if (res != MOJO_RESULT_OK) {
    LOG(ERROR) << "Failed to query shared buffer info (res = " << res << ")";
    Reset();
    return;
  }

  // Invalid size?  Close the connection.
  uint64_t size = info.num_bytes;
  if (!size || (size > MediaConsumer::kMaxBufferLen)) {
    LOG(ERROR) << "Invalid shared buffer size (size = " << size << ")";
    Reset();
    return;
  }

  // Failed to map the buffer?  Close the connection.
  buffer_ = MappedSharedBuffer::Create(handle.Pass(), size);
  if (!buffer_) {
    LOG(ERROR) << "Failed to map shared memory buffer (size = " << size << ")";
    Reset();
    return;
  }

  cbk.Run();
}

void MediaPipeBase::SendPacket(MediaPacketPtr packet,
                               const SendPacketCallback& cbk) {
  // If we have not been successfully initialized, then we should not be getting
  // packets pushed to us.
  if (!buffer_) {
    LOG(ERROR) << "SendPacket called with no shared buffer established!";
    Reset();
    return;
  }
  DCHECK(buffer_->size());

  // The offset(s) and size(s) of this payload must to reside within the space
  // of the shared buffer.  If any does not, this send operation is not valid.
  const MediaPacketRegionPtr* r = &packet->payload;
  size_t i = 0;
  size_t extra_payload_size =
      packet->extra_payload.is_null() ? 0 : packet->extra_payload.size();
  size_t total_length = 0;
  while (true) {
    if ((*r).is_null()) {
      LOG(ERROR) << "Missing region structure at index " << i
                 << " during SendPacket";
      Reset();
      return;
    }

    auto offset = (*r)->offset;
    auto length = (*r)->length;
    if ((offset > buffer_->size()) || (length > (buffer_->size() - offset))) {
      LOG(ERROR) << "Region [" << offset << "," << (offset + length)
                 << ") at index " << i
                 << " is out of range for shared buffer of size "
                 << buffer_->size()
                 << " during SendPacket.";
      Reset();
      return;
    }

    total_length += length;

    if (i >= extra_payload_size) {
      break;
    }

    DCHECK(packet->extra_payload);
    r = &packet->extra_payload[i++];
  }

  // Looks good, send this packet up to the implementation layer.
  MediaPacketStatePtr ptr(new MediaPacketState(packet.Pass(), buffer_, cbk));
  OnPacketReceived(std::move(ptr));
}

void MediaPipeBase::Prime(const PrimeCallback& cbk) {
  OnPrimeRequested(cbk);
}

void MediaPipeBase::Flush(const FlushCallback& cbk) {
  // If we have not been successfully initialized, then we should not be getting
  // packets pushed to us.
  if (!buffer_) {
    LOG(ERROR) << "Flush called with no shared buffer established!";
    Reset();
    return;
  }

  // Pass the flush request up to the implementation layer.  If something goes
  // fatally wrong up there, close the connection.
  if (!OnFlushRequested(cbk)) {
    Reset();
  }
}

MediaPipeBase::MediaPacketState::MediaPacketState(
    MediaPacketPtr packet,
    const MappedSharedBufferPtr& buffer,
    const SendPacketCallback& cbk)
  : packet_(packet.Pass()),
    buffer_(buffer),
    cbk_(cbk),
    result_(MediaConsumer::SendResult::CONSUMED) {
  DCHECK(packet_);
  DCHECK(packet_->payload);
}

MediaPipeBase::MediaPacketState::~MediaPacketState() {
  cbk_.Run(result_);
}

void MediaPipeBase::MediaPacketState::SetResult(
    MediaConsumer::SendResult result) {
  MediaConsumer::SendResult tmp = MediaConsumer::SendResult::CONSUMED;
  result_.compare_exchange_strong(tmp, result);
}

MediaPipeBase::MappedSharedBufferPtr MediaPipeBase::MappedSharedBuffer::Create(
    ScopedSharedBufferHandle handle,
    uint64_t size) {
  MappedSharedBufferPtr ret(new MappedSharedBuffer(handle.Pass(), size));
  return ret->base() ? ret : nullptr;
}

MediaPipeBase::MappedSharedBuffer::~MappedSharedBuffer() {
  if (nullptr != base_) {
    MojoResult res = UnmapBuffer(base_);
    CHECK(res == MOJO_RESULT_OK);
  }
}

MediaPipeBase::MappedSharedBuffer::MappedSharedBuffer(
    ScopedSharedBufferHandle handle,
    uint64_t size)
  : handle_(handle.Pass()),
    size_(size) {
  MojoResult res = MapBuffer(handle_.get(),
                             0u,
                             size_,
                             &base_,
                             MOJO_MAP_BUFFER_FLAG_NONE);

  if (MOJO_RESULT_OK != res) {
    LOG(ERROR) << "Failed to map shared buffer of size " << size_
               << " (res " << res << ")";
    DCHECK(base_ == nullptr);
    handle_.reset();
  }
}

}  // namespace media
}  // namespace mojo
