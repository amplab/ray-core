// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/platform/native/platform_handle_private_thunks.h"

#include "mojo/public/platform/native/thunk_export.h"

#include <assert.h>

static struct MojoPlatformHandlePrivateThunks g_thunks = {0};

MojoResult MojoCreatePlatformHandleWrapper(
    MojoPlatformHandle platform_handle,
    MojoHandle* platform_handle_wrapper_handle) {
  assert(g_thunks.CreatePlatformHandleWrapper);
  return g_thunks.CreatePlatformHandleWrapper(platform_handle,
                                              platform_handle_wrapper_handle);
}

MojoResult MojoExtractPlatformHandle(MojoHandle platform_handle_wrapper_handle,
                                     MojoPlatformHandle* platform_handle) {
  assert(g_thunks.ExtractPlatformHandle);
  return g_thunks.ExtractPlatformHandle(platform_handle_wrapper_handle,
                                        platform_handle);
}

THUNK_EXPORT size_t MojoSetPlatformHandlePrivateThunks(
    const struct MojoPlatformHandlePrivateThunks* thunks) {
  if (thunks->size >= sizeof(g_thunks))
    g_thunks = *thunks;
  return sizeof(g_thunks);
}
