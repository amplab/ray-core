// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_MEDIA_MODELS_ACTIVE_SOURCE_H_
#define MOJO_MEDIA_MODELS_ACTIVE_SOURCE_H_

#include "services/media/framework/models/demand.h"
#include "services/media/framework/models/part.h"
#include "services/media/framework/packet.h"
#include "services/media/framework/payload_allocator.h"

namespace mojo {
namespace media {

// Source that produces packets asynchronously.
class ActiveSource : public Part {
 public:
  using SupplyCallback = std::function<void(PacketPtr packet)>;

  ~ActiveSource() override {}

  // Whether the source can accept an allocator.
  virtual bool can_accept_allocator() const = 0;

  // Sets the allocator for the source.
  virtual void set_allocator(PayloadAllocator* allocator) = 0;

  // Sets the callback that supplies a packet asynchronously.
  virtual void SetSupplyCallback(const SupplyCallback& supply_callback) = 0;

  // Sets the demand signalled from downstream.
  virtual void SetDownstreamDemand(Demand demand) = 0;
};

}  // namespace media
}  // namespace mojo

#endif  // MOJO_MEDIA_MODELS_ACTIVE_SOURCE_H_
