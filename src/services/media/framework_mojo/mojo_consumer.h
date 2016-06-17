// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_CONSUMER_H_
#define SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_CONSUMER_H_

#include "base/single_thread_task_runner.h"
#include "base/task_runner.h"
#include "mojo/common/binding_set.h"
#include "mojo/services/media/common/cpp/mapped_shared_buffer.h"
#include "mojo/services/media/common/interfaces/media_transport.mojom.h"
#include "services/media/framework/models/active_source.h"

namespace mojo {
namespace media {

// Implements MediaConsumer::Flush on behalf of MediaConsumer to avoid name
// conflict with Part::Flush.
class MojoConsumerMediaConsumer : public MediaConsumer {
  // MediaConsumer implementation.
  void Flush(const FlushCallback& callback) override;

  // Implements MediaConsumer::Flush.
  virtual void MediaConsumerFlush(const FlushCallback& callback) = 0;
};

// Implements MediaConsumer to receive a stream from across mojo.
class MojoConsumer : public MojoConsumerMediaConsumer, public ActiveSource {
 public:
  using PrimeRequestedCallback = std::function<void(const PrimeCallback&)>;
  using FlushRequestedCallback = std::function<void(const FlushCallback&)>;

  static std::shared_ptr<MojoConsumer> Create() {
    return std::shared_ptr<MojoConsumer>(new MojoConsumer());
  }

  ~MojoConsumer() override;

  // Adds a binding.
  void AddBinding(InterfaceRequest<MediaConsumer> consumer);

  // Sets a callback signalling that a prime has been requested from the
  // MediaConsumer client.
  void SetPrimeRequestedCallback(const PrimeRequestedCallback& callback);

  // Sets a callback signalling that a flush has been requested from the
  // MediaConsumer client.
  void SetFlushRequestedCallback(const FlushRequestedCallback& callback);

  // MediaConsumer implementation.
  void SetBuffer(ScopedSharedBufferHandle buffer,
                 const SetBufferCallback& callback) override;

  void SendPacket(MediaPacketPtr packet,
                  const SendPacketCallback& callback) override;

  void Prime(const PrimeCallback& callback) override;

  void MediaConsumerFlush(const FlushCallback& callback) override;

  // ActiveSource implementation.
  bool can_accept_allocator() const override;

  void set_allocator(PayloadAllocator* allocator) override;

  void SetSupplyCallback(const SupplyCallback& supply_callback) override;

  void SetDownstreamDemand(Demand demand) override;

 private:
  MojoConsumer();

  // Specialized packet implementation.
  class PacketImpl : public Packet {
   public:
    static PacketPtr Create(
        MediaPacketPtr media_packet,
        const SendPacketCallback& callback,
        scoped_refptr<base::SingleThreadTaskRunner> task_runner,
        const MappedSharedBuffer& buffer) {
      return PacketPtr(
          new PacketImpl(media_packet.Pass(), callback, task_runner, buffer));
    }

   protected:
    void Release() override;

   private:
    PacketImpl(MediaPacketPtr media_packet,
               const SendPacketCallback& callback,
               scoped_refptr<base::SingleThreadTaskRunner> task_runner,
               const MappedSharedBuffer& buffer);

    ~PacketImpl() override;

    static void RunCallback(const SendPacketCallback& callback);

    MediaPacketPtr media_packet_;
    const SendPacketCallback callback_;
    scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  };

  BindingSet<MediaConsumer> bindings_;
  PrimeRequestedCallback prime_requested_callback_;
  FlushRequestedCallback flush_requested_callback_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  MappedSharedBuffer buffer_;
  SupplyCallback supply_callback_;
};

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_CONSUMER_H_
