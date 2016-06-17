// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/media/framework_mojo/mojo_allocator.h"

namespace mojo {
namespace media {

MojoAllocator::~MojoAllocator() {}

void* MojoAllocator::AllocatePayloadBuffer(size_t size) {
  return AllocateRegion(size);
}

void MojoAllocator::ReleasePayloadBuffer(size_t size, void* buffer) {
  ReleaseRegion(size, buffer);
}

}  // namespace media
}  // namespace mojo
