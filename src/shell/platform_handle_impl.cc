// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains implementations for the platform handle thunks
// (see //mojo/public/platform/native/platform_handle_private.h)

#include "base/logging.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/platform/native/platform_handle_private.h"

using mojo::platform::PlatformHandle;
using mojo::platform::ScopedPlatformHandle;

MojoResult MojoCreatePlatformHandleWrapper(MojoPlatformHandle platform_handle,
                                           MojoHandle* wrapper) {
  PlatformHandle platform_handle_wrapper(platform_handle);
  ScopedPlatformHandle scoped_platform_handle(platform_handle_wrapper);
  return mojo::embedder::CreatePlatformHandleWrapper(
      scoped_platform_handle.Pass(), wrapper);
}

MojoResult MojoExtractPlatformHandle(MojoHandle wrapper,
                                     MojoPlatformHandle* platform_handle) {
  ScopedPlatformHandle scoped_platform_handle;
  MojoResult result = mojo::embedder::PassWrappedPlatformHandle(
      wrapper, &scoped_platform_handle);
  if (result != MOJO_RESULT_OK)
    return result;

  DCHECK(scoped_platform_handle.is_valid());
  *platform_handle = scoped_platform_handle.release().fd;
  return MOJO_RESULT_OK;
}
