// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_GPU_MGL_MGL_ECHO_H_
#define MOJO_PUBLIC_C_GPU_MGL_MGL_ECHO_H_

#include <stdint.h>

#include "mojo/public/c/gpu/MGL/mgl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*MGLEchoCallback)(void* closure);

void MGLEcho(MGLEchoCallback callback, void* closure);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // MOJO_PUBLIC_C_GPU_MGL_MGL_ECHO_H_
