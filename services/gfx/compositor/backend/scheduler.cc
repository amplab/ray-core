// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/gfx/compositor/backend/scheduler.h"

namespace compositor {

SchedulerCallbacks::SchedulerCallbacks(const FrameCallback& update_callback,
                                       const FrameCallback& snapshot_callback)
    : update_callback(update_callback), snapshot_callback(snapshot_callback) {}

SchedulerCallbacks::~SchedulerCallbacks() {}

}  // namespace compositor
