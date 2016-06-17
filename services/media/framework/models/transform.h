// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_MEDIA_MODELS_TRANSFORM_H_
#define MOJO_MEDIA_MODELS_TRANSFORM_H_

#include "services/media/framework/models/part.h"
#include "services/media/framework/packet.h"
#include "services/media/framework/payload_allocator.h"

namespace mojo {
namespace media {

// Synchronous packet transform.
class Transform : public Part {
 public:
  ~Transform() override {}

  // Processes a packet. Returns true to indicate the transform is done
  // processing the input packet. Returns false to indicate the input
  // packet should be processed again. new_input indicates whether the input
  // packet is new (true) or is being processed again (false). An output packet
  // may or may not be generated for any given invocation of this method.
  virtual bool TransformPacket(const PacketPtr& input,
                               bool new_input,
                               PayloadAllocator* allocator,
                               PacketPtr* output) = 0;
};

}  // namespace media
}  // namespace mojo

#endif  // MOJO_MEDIA_MODELS_TRANSFORM_H_
