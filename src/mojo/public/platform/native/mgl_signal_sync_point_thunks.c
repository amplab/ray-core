// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/platform/native/mgl_signal_sync_point_thunks.h"

#include <assert.h>

#include "mojo/public/platform/native/thunk_export.h"

static struct MGLSignalSyncPointThunks g_signal_sync_point_thunks = {0};

void MGLSignalSyncPoint(uint32_t sync_point,
                        MGLSignalSyncPointCallback callback,
                        void* closure) {
  assert(g_signal_sync_point_thunks.MGLSignalSyncPoint);
  g_signal_sync_point_thunks.MGLSignalSyncPoint(sync_point, callback, closure);
}

THUNK_EXPORT size_t MojoSetMGLSignalSyncPointThunks(
    const struct MGLSignalSyncPointThunks* mgl_signal_sync_point_thunks) {
  if (mgl_signal_sync_point_thunks->size >= sizeof(g_signal_sync_point_thunks))
    g_signal_sync_point_thunks = *mgl_signal_sync_point_thunks;
  return sizeof(g_signal_sync_point_thunks);
}
