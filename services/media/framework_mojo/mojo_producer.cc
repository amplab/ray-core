// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "services/media/framework_mojo/mojo_producer.h"

namespace mojo {
namespace media {

MojoProducer::MojoProducer() {
  task_runner_ = base::MessageLoop::current()->task_runner();
  DCHECK(task_runner_);
}

MojoProducer::~MojoProducer() {
  base::AutoLock lock(lock_);
}

void MojoProducer::AddBinding(InterfaceRequest<MediaProducer> producer) {
  bindings_.AddBinding(this, producer.Pass());
}

void MojoProducer::PrimeConnection(const PrimeConnectionCallback& callback) {
  Demand demand;

  if (consumer_.is_bound()) {
    base::AutoLock lock(lock_);
    max_pushes_outstanding_ = 3;  // TODO(dalesat): Made up!
    demand = current_pushes_outstanding_ < max_pushes_outstanding_
                 ? Demand::kPositive
                 : Demand::kNegative;
  } else {
    demand = Demand::kNeutral;
    if (!mojo_allocator_.initialized()) {
      mojo_allocator_.InitNew(2048 * 1024);  // TODO(dalesat): Made up!
    }
  }

  DCHECK(demand_callback_);
  demand_callback_(demand);

  if (consumer_.is_bound()) {
    consumer_->Prime([this, callback]() { callback.Run(); });
  } else {
    callback.Run();
  }
}

void MojoProducer::FlushConnection(const FlushConnectionCallback& callback) {
  {
    base::AutoLock lock(lock_);
    max_pushes_outstanding_ = 0;
  }

  DCHECK(demand_callback_);
  demand_callback_(Demand::kNegative);

  if (consumer_.is_bound()) {
    consumer_->Flush(callback);
  } else {
    callback.Run();
  }
}

PayloadAllocator* MojoProducer::allocator() {
  return &mojo_allocator_;
}

void MojoProducer::SetDemandCallback(const DemandCallback& demand_callback) {
  demand_callback_ = demand_callback;
}

Demand MojoProducer::SupplyPacket(PacketPtr packet) {
  DCHECK(packet);

  // If we're not connected, throw the packet away.
  if (!consumer_.is_bound()) {
    return packet->end_of_stream() ? Demand::kNegative : Demand::kNeutral;
  }

  Demand demand;

  {
    base::AutoLock lock(lock_);
    DCHECK(current_pushes_outstanding_ < max_pushes_outstanding_);

    ++current_pushes_outstanding_;

    if (packet->end_of_stream()) {
      demand = Demand::kNegative;
      max_pushes_outstanding_ = 0;
    } else {
      demand = current_pushes_outstanding_ < max_pushes_outstanding_
                   ? Demand::kPositive
                   : Demand::kNegative;
    }
  }

  MediaPacketPtr media_packet = CreateMediaPacket(packet);
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MojoProducer::SendPacket, base::Unretained(this),
                 packet.release(), base::Passed(media_packet.Pass())));

  return demand;
}

void MojoProducer::Connect(InterfaceHandle<MediaConsumer> consumer,
                           const ConnectCallback& callback) {
  DCHECK(consumer);

  consumer_ = MediaConsumerPtr::Create(std::move(consumer));

  if (!mojo_allocator_.initialized()) {
    mojo_allocator_.InitNew(2048 * 1024);  // TODO(dalesat): Made up!
  }

  consumer_->SetBuffer(mojo_allocator_.GetDuplicateHandle(),
                       [callback]() { callback.Run(); });
}

void MojoProducer::Disconnect() {
  DCHECK(demand_callback_);
  demand_callback_(Demand::kNegative);
  consumer_.reset();
}

void MojoProducer::SendPacket(Packet* packet_raw_ptr,
                              MediaPacketPtr media_packet) {
  consumer_->SendPacket(
      media_packet.Pass(),
      [this, packet_raw_ptr](MediaConsumer::SendResult send_result) {
        PacketPtr packet = PacketPtr(packet_raw_ptr);
        Demand demand;

        {
          base::AutoLock lock(lock_);
          demand = --current_pushes_outstanding_ < max_pushes_outstanding_
                       ? Demand::kPositive
                       : Demand::kNegative;
        }

        DCHECK(demand_callback_);
        demand_callback_(demand);
      });
}

MediaPacketPtr MojoProducer::CreateMediaPacket(const PacketPtr& packet) {
  DCHECK(packet);

  MediaPacketRegionPtr region = MediaPacketRegion::New();
  region->offset = packet->size() == 0
                       ? 0
                       : mojo_allocator_.OffsetFromPtr(packet->payload());
  region->length = packet->size();

  MediaPacketPtr media_packet = MediaPacket::New();
  media_packet->pts = packet->pts();
  media_packet->end_of_stream = packet->end_of_stream();
  media_packet->payload = region.Pass();

  return media_packet.Pass();
}

}  // namespace media
}  // namespace mojo
