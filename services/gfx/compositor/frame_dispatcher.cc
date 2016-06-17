// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/gfx/compositor/frame_dispatcher.h"

namespace compositor {

FrameDispatcher::FrameDispatcher() {}

FrameDispatcher::~FrameDispatcher() {}

bool FrameDispatcher::AddCallback(const FrameCallback& callback) {
  pending_callbacks_.emplace_back(callback);
  return pending_callbacks_.size() == 1u;
}

void FrameDispatcher::DispatchCallbacks(
    const mojo::gfx::composition::FrameInfo& frame_info) {
  for (auto& callback : pending_callbacks_) {
    callback.Run(frame_info.Clone());
  }
  pending_callbacks_.clear();
}

}  // namespace compositor
