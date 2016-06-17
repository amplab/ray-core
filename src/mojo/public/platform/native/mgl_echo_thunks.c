// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/platform/native/mgl_echo_thunks.h"

#include <assert.h>

#include "mojo/public/platform/native/thunk_export.h"

static struct MGLEchoThunks g_echo_thunks = {0};

void MGLEcho(MGLEchoCallback callback, void* closure) {
  assert(g_echo_thunks.MGLEcho);
  g_echo_thunks.MGLEcho(callback, closure);
}

THUNK_EXPORT size_t MojoSetMGLEchoThunks(
    const struct MGLEchoThunks* mgl_echo_thunks) {
  if (mgl_echo_thunks->size >= sizeof(g_echo_thunks))
    g_echo_thunks = *mgl_echo_thunks;
  return sizeof(g_echo_thunks);
}
