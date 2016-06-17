// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_PULL_MODE_PRODUCER_H_
#define SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_PULL_MODE_PRODUCER_H_

#include <deque>

#include "base/synchronization/lock.h"
#include "mojo/common/binding_set.h"
#include "mojo/services/media/common/interfaces/media_transport.mojom.h"
#include "services/media/framework/models/active_sink.h"
#include "services/media/framework_mojo/mojo_allocator.h"

namespace mojo {
namespace media {

// Implements MediaPullModeProducer to forward a stream across mojo.
class MojoPullModeProducer : public MediaPullModeProducer, public ActiveSink {
 public:
  static std::shared_ptr<MojoPullModeProducer> Create() {
    return std::shared_ptr<MojoPullModeProducer>(new MojoPullModeProducer());
  }

  ~MojoPullModeProducer() override;

  // Adds a binding.
  void AddBinding(InterfaceRequest<MediaPullModeProducer> producer);

  // MediaPullModeProducer implementation.
  void GetBuffer(const GetBufferCallback& callback) override;

  void PullPacket(MediaPacketPtr to_release,
                  const PullPacketCallback& callback) override;

  void ReleasePacket(MediaPacketPtr to_release) override;

  // ActiveSink implementation.
  PayloadAllocator* allocator() override;

  void SetDemandCallback(const DemandCallback& demand_callback) override;

  Demand SupplyPacket(PacketPtr packet) override;

 private:
  MojoPullModeProducer();

  // Handles as many pending pulls as possible.
  // MUST BE CALLED WITH lock_ TAKEN.
  void HandlePendingPullsUnsafe();

  // Attempts to handle a pull and indicates whether it was handled.
  // MUST BE CALLED WITH lock_ TAKEN.
  bool MaybeHandlePullUnsafe(const PullPacketCallback& callback);

  // Runs the callback with a new MediaPacket created from the given Packet.
  // MUST BE CALLED WITH lock_ TAKEN.
  void HandlePullWithPacketUnsafe(const PullPacketCallback& callback,
                                  PacketPtr packet);

  // Creates a MediaPacket from a Packet.
  MediaPacketPtr CreateMediaPacket(const PacketPtr& packet);

  BindingSet<MediaPullModeProducer> bindings_;

  DemandCallback demand_callback_;

  // Allocates from the shared buffer.
  MojoAllocator mojo_allocator_;

  mutable base::Lock lock_;
  // THE FIELDS BELOW SHOULD ONLY BE ACCESSED WITH lock_ TAKEN.
  Demand demand_;
  int64_t pts_;

  // pending_pulls_ contains the callbacks for the pull requests that have yet
  // to be satisfied. unreleased_packets_ contains the packets that have been
  // delivered via pull but have not yet been released. cached_packet_ is a
  // packet waiting for a pull (we keep one ready so we can be preparing a
  // packet between pulls). If cached_packet_ isn't nullptr, pending_pulls_
  // should be empty. We signal positive demand when cached_packet_ is nullptr
  // and negative demand when it isn't.
  std::deque<PullPacketCallback> pending_pulls_;
  std::deque<PacketPtr> unreleased_packets_;
  PacketPtr cached_packet_;
  // THE FIELDS ABOVE SHOULD ONLY BE ACCESSED WITH lock_ TAKEN.
};

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_PULL_MODE_PRODUCER_H_
