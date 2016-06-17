// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_GPU_MGL_MGL_SIGNAL_SYNC_POINT_H_
#define MOJO_PUBLIC_C_GPU_MGL_MGL_SIGNAL_SYNC_POINT_H_

#include <stdint.h>

#include "mojo/public/c/gpu/MGL/mgl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*MGLSyncPointCallback)(void* closure);

// MGLSignalSyncPoint signals the given sync point in the current context and
// invokes |callback| when the service side acknowleges that the sync point has
// been passed.
void MGLSignalSyncPoint(uint32_t sync_point,
                        MGLSignalSyncPointCallback callback,
                        void* closure);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // MOJO_PUBLIC_C_GPU_MGL_MGL_SIGNAL_SYNC_POINT_H_
