// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_PLATFORM_NACL_MGL_IRT_H_
#define MOJO_PUBLIC_PLATFORM_NACL_MGL_IRT_H_

#include "mojo/public/c/gpu/MGL/mgl.h"
#include "mojo/public/c/gpu/MGL/mgl_onscreen.h"
#include "mojo/public/c/gpu/MGL/mgl_signal_sync_point.h"

#define NACL_IRT_MGL_v0_1 "nacl-irt-mgl-0.1"

// See mojo/public/c/gpu/MGL/mgl.h for documentation.
struct nacl_irt_mgl {
  MGLContext (*MGLCreateContext)(MGLOpenGLAPIVersion version,
                                 MojoHandle command_buffer_handle,
                                 MGLContext share_group,
                                 MGLContextLostCallback lost_callback,
                                 void* lost_callback_closure,
                                 const struct MojoAsyncWaiter* async_waiter);
  void (*MGLDestroyContext)(MGLContext context);
  void (*MGLMakeCurrent)(MGLContext context);
  MGLContext (*MGLGetCurrentContext)(void);
  MGLMustCastToProperFunctionPointerType (*MGLGetProcAddress)(const char* name);
};

#define NACL_IRT_MGL_ONSCREEN_v0_1 "nacl-irt-mgl-onscreen-0.1"

// See mojo/public/c/gpu/MGL/mgl_onscreen.h for documentation.
struct nacl_irt_mgl_onscreen {
  void (*MGLResizeSurface)(uint32_t width, uint32_t height);
  void (*MGLSwapBuffers)(void);
};

#define NACL_IRT_MGL_SIGNAL_SYNC_POINT_v0_1 "nacl-irt-mgl-signal-sync-point-0.1"

// See mojo/public/c/gpu/MGL/mgl_signal_sync_point.h for documentation.
struct nacl_irt_mgl_signal_sync_point {
  void (*MGLSignalSyncPoint)(uint32_t sync_point,
                             MGLSignalSyncPointCallback callback,
                             void* closure);
};

#endif  // MOJO_PUBLIC_PLATFORM_NACL_MGL_IRT_H_
