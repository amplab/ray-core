// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "services/media/framework_mojo/mojo_pull_mode_producer.h"

namespace mojo {
namespace media {

MojoPullModeProducer::MojoPullModeProducer()
    : demand_(Demand::kNegative),
      pts_(0),
      cached_packet_(nullptr) {}

MojoPullModeProducer::~MojoPullModeProducer() {
  base::AutoLock lock(lock_);
}

void MojoPullModeProducer::AddBinding(
    InterfaceRequest<MediaPullModeProducer> producer) {
  bindings_.AddBinding(this, producer.Pass());
}

void MojoPullModeProducer::GetBuffer(const GetBufferCallback& callback) {
  if (!mojo_allocator_.initialized()) {
    mojo_allocator_.InitNew(256 * 1024);  // TODO(dalesat): Made up!
  }

  callback.Run(mojo_allocator_.GetDuplicateHandle());

  DCHECK(!cached_packet_);
  DCHECK(demand_callback_);
  demand_callback_(Demand::kPositive);
}

void MojoPullModeProducer::PullPacket(MediaPacketPtr to_release,
                                      const PullPacketCallback& callback) {
  if (to_release) {
    // The client has piggy-backed a release on this pull request.
    ReleasePacket(to_release.Pass());
  }

  {
    base::AutoLock lock(lock_);

    //if (state_ == MediaState::UNPREPARED) {
      // The consumer has yet to call GetBuffer. This request will have to wait.
    //  pending_pulls_.push_back(callback);
    //  return;
    //}

    DCHECK(mojo_allocator_.initialized());

    // If there are no pending requests, see if we can handle this now. If
    // requests are pending, add the callback to the pending queue.
    if (!pending_pulls_.empty() || !MaybeHandlePullUnsafe(callback)) {
      pending_pulls_.push_back(callback);
    }

    DCHECK(!cached_packet_);
  }

  DCHECK(demand_callback_);
  demand_callback_(Demand::kPositive);
}

void MojoPullModeProducer::ReleasePacket(MediaPacketPtr to_release) {
  {
    base::AutoLock lock(lock_);
    uint64_t size = to_release->payload ? to_release->payload->length : 0;
    void* payload = size == 0 ? nullptr : mojo_allocator_.PtrFromOffset(
                                              to_release->payload->offset);

    for (auto iterator = unreleased_packets_.begin(); true; ++iterator) {
      if (iterator == unreleased_packets_.end()) {
        DCHECK(false) << "released packet has bad offset and/or size";
        break;
      }

      if ((*iterator)->payload() == payload && (*iterator)->size() == size) {
        unreleased_packets_.erase(iterator);
        break;
      }
    }

    // TODO(dalesat): What if the allocator has starved?
  }

  DCHECK(demand_callback_);
  demand_callback_(cached_packet_ ? Demand::kNegative : Demand::kPositive);
}

PayloadAllocator* MojoPullModeProducer::allocator() {
  return mojo_allocator_.initialized() ? &mojo_allocator_ : nullptr;
}

void MojoPullModeProducer::SetDemandCallback(
    const DemandCallback& demand_callback) {
  demand_callback_ = demand_callback;
}

Demand MojoPullModeProducer::SupplyPacket(PacketPtr packet) {
  base::AutoLock lock(lock_);
  DCHECK(demand_ != Demand::kNegative) << "packet pushed with negative demand";

  DCHECK(!cached_packet_);

  // If there's no binding on the stream, throw the packet away. This can
  // happen if a pull client disconnects unexpectedly.
  if (bindings_.size() == 0) {
    demand_ = Demand::kNegative;
    // TODO(dalesat): More shutdown?
    return demand_;
  }

  // Accept the packet and handle pending pulls with it.
  cached_packet_ = std::move(packet);

  HandlePendingPullsUnsafe();

  demand_ = cached_packet_ ? Demand::kNegative : Demand::kPositive;
  return demand_;
}

void MojoPullModeProducer::HandlePendingPullsUnsafe() {
  lock_.AssertAcquired();

  while (!pending_pulls_.empty()) {
    DCHECK(mojo_allocator_.initialized());

    if (MaybeHandlePullUnsafe(pending_pulls_.front())) {
      pending_pulls_.pop_front();
    } else {
      break;
    }
  }
}

bool MojoPullModeProducer::MaybeHandlePullUnsafe(
    const PullPacketCallback& callback) {
  DCHECK(!callback.is_null());
  lock_.AssertAcquired();

  //if (state_ == MediaState::ENDED) {
    // At end-of-stream. Respond with empty end-of-stream packet.
  //  HandlePullWithPacketUnsafe(callback, Packet::CreateEndOfStream(pts_));
  //  return true;
  //}

  if (!cached_packet_) {
    // Waiting for packet or end-of-stream indication.
    return false;
  }

  HandlePullWithPacketUnsafe(callback, std::move(cached_packet_));
  return true;
}

void MojoPullModeProducer::HandlePullWithPacketUnsafe(
    const PullPacketCallback& callback,
    PacketPtr packet) {
  DCHECK(packet);
  lock_.AssertAcquired();

  // TODO(dalesat): Use TaskRunner for this callback.
  callback.Run(CreateMediaPacket(packet));
  unreleased_packets_.push_back(std::move(packet));
}

MediaPacketPtr MojoPullModeProducer::CreateMediaPacket(
    const PacketPtr& packet) {
  DCHECK(packet);

  MediaPacketRegionPtr region = MediaPacketRegion::New();
  region->offset = mojo_allocator_.OffsetFromPtr(packet->payload());
  region->length = packet->size();

  MediaPacketPtr media_packet = MediaPacket::New();
  media_packet->pts = packet->pts();
  media_packet->end_of_stream = packet->end_of_stream();
  media_packet->payload = region.Pass();
  pts_ = packet->pts();

  return media_packet.Pass();
}

}  // namespace media
}  // namespace mojo
