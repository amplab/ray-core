// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "services/media/framework_mojo/mojo_consumer.h"

namespace mojo {
namespace media {

void MojoConsumerMediaConsumer::Flush(const FlushCallback& callback) {
  MediaConsumerFlush(callback);
}

MojoConsumer::MojoConsumer() {}

MojoConsumer::~MojoConsumer() {}

void MojoConsumer::AddBinding(InterfaceRequest<MediaConsumer> consumer) {
  bindings_.AddBinding(this, consumer.Pass());
  DCHECK(base::MessageLoop::current());
  task_runner_ = base::MessageLoop::current()->task_runner();
  DCHECK(task_runner_);
}

void MojoConsumer::SetPrimeRequestedCallback(
    const PrimeRequestedCallback& callback) {
  prime_requested_callback_ = callback;
}

void MojoConsumer::SetFlushRequestedCallback(
    const FlushRequestedCallback& callback) {
  flush_requested_callback_ = callback;
}

void MojoConsumer::SetBuffer(ScopedSharedBufferHandle buffer,
                             const SetBufferCallback& callback) {
  buffer_.InitFromHandle(buffer.Pass());
  callback.Run();
}

void MojoConsumer::SendPacket(MediaPacketPtr media_packet,
                              const SendPacketCallback& callback) {
  DCHECK(media_packet);
  DCHECK(supply_callback_);
  supply_callback_(
      PacketImpl::Create(media_packet.Pass(), callback, task_runner_, buffer_));
}

void MojoConsumer::Prime(const PrimeCallback& callback) {
  if (prime_requested_callback_) {
    prime_requested_callback_(callback);
  } else {
    LOG(WARNING) << "prime requested but no callback registered";
    callback.Run();
  }
}

void MojoConsumer::MediaConsumerFlush(const FlushCallback& callback) {
  if (flush_requested_callback_) {
    flush_requested_callback_(callback);
  } else {
    LOG(WARNING) << "flush requested but no callback registered";
    callback.Run();
  }
}

bool MojoConsumer::can_accept_allocator() const {
  return false;
}

void MojoConsumer::set_allocator(PayloadAllocator* allocator) {
  LOG(ERROR) << "set_allocator called on MojoConsumer";
}

void MojoConsumer::SetSupplyCallback(const SupplyCallback& supply_callback) {
  supply_callback_ = supply_callback;
}

void MojoConsumer::SetDownstreamDemand(Demand demand) {}

MojoConsumer::PacketImpl::PacketImpl(
    MediaPacketPtr media_packet,
    const SendPacketCallback& callback,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    const MappedSharedBuffer& buffer)
    : Packet(media_packet->pts,
             media_packet->end_of_stream,
             media_packet->payload->length,
             media_packet->payload->length == 0
                 ? nullptr
                 : buffer.PtrFromOffset(media_packet->payload->offset)),
      media_packet_(media_packet.Pass()),
      callback_(callback),
      task_runner_(task_runner) {}

MojoConsumer::PacketImpl::~PacketImpl() {}

// static
void MojoConsumer::PacketImpl::RunCallback(const SendPacketCallback& callback) {
  callback.Run(MediaConsumer::SendResult::CONSUMED);
}

void MojoConsumer::PacketImpl::Release() {
  task_runner_->PostTask(FROM_HERE, base::Bind(&RunCallback, callback_));
  delete this;
}

}  // namespace media
}  // namespace mojo
