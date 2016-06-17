// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_GFX_COMPOSITOR_FRAME_DISPATCHER_H_
#define SERVICES_GFX_COMPOSITOR_FRAME_DISPATCHER_H_

#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "mojo/services/gfx/composition/interfaces/scheduling.mojom.h"

namespace compositor {

using FrameCallback =
    base::Callback<void(mojo::gfx::composition::FrameInfoPtr)>;

// Maintains a list of pending frame callbacks to be dispatched.
class FrameDispatcher {
 public:
  FrameDispatcher();
  ~FrameDispatcher();

  // Adds a callback, returns true if it was the first pending callback.
  bool AddCallback(const FrameCallback& callback);

  // Dispatches all pending callbacks then clears the list.
  void DispatchCallbacks(const mojo::gfx::composition::FrameInfo& frame_info);

 private:
  std::vector<FrameCallback> pending_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(FrameDispatcher);
};

}  // namespace compositor

#endif  // SERVICES_GFX_COMPOSITOR_FRAME_DISPATCHER_H_
