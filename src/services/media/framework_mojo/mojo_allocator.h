// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_ALLOCATOR_H_
#define SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_ALLOCATOR_H_

#include "mojo/services/media/common/cpp/shared_media_buffer_allocator.h"
#include "services/media/framework/payload_allocator.h"

namespace mojo {
namespace media {

// Just like SharedMediaBufferAllocator but implements PayloadAllocator.
class MojoAllocator : public SharedMediaBufferAllocator,
                      public PayloadAllocator {
 public:
  ~MojoAllocator() override;

  // PayloadAllocator implementation.
  void* AllocatePayloadBuffer(size_t size) override;

  void ReleasePayloadBuffer(size_t size, void* buffer) override;
};

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_ALLOCATOR_H_
