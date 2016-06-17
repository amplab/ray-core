// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/platform/nacl/mgl_irt.h"
#include "native_client/src/untrusted/irt/irt.h"

namespace {

bool g_irt_mgl_valid = false;
struct nacl_irt_mgl g_irt_mgl;

struct nacl_irt_mgl* GetIrtMGL() {
  if (!g_irt_mgl_valid) {
    size_t rc = nacl_interface_query(NACL_IRT_MGL_v0_1, &g_irt_mgl,
                                     sizeof(g_irt_mgl));
    if (rc != sizeof(g_irt_mgl))
      return nullptr;
    g_irt_mgl_valid = true;
  }
  return &g_irt_mgl;
}

bool g_irt_mgl_onscreen_valid = false;
struct nacl_irt_mgl_onscreen g_irt_mgl_onscreen;

struct nacl_irt_mgl_onscreen* GetIrtMGLOnScreen() {
  if (!g_irt_mgl_onscreen_valid) {
    size_t rc = nacl_interface_query(NACL_IRT_MGL_ONSCREEN_v0_1,
                                     &g_irt_mgl_onscreen,
                                     sizeof(g_irt_mgl_onscreen));
    if (rc != sizeof(g_irt_mgl_onscreen))
      return nullptr;
    g_irt_mgl_onscreen_valid = true;
  }
  return &g_irt_mgl_onscreen;
}

bool g_irt_mgl_signal_sync_point_valid = false;
struct nacl_irt_mgl_signal_sync_point g_irt_mgl_signal_sync_point;

struct nacl_irt_mgl_signal_sync_point* GetIrtMGLSignalSyncPoint() {
  if (!g_irt_mgl_signal_sync_point_valid) {
    size_t rc = nacl_interface_query(NACL_IRT_MGL_SIGNAL_SYNC_POINT_v0_1,
                                     &g_irt_mgl_signal_sync_point,
                                     sizeof(g_irt_mgl_signal_sync_point));
    if (rc != sizeof(g_irt_mgl_signal_sync_point))
      return nullptr;
    g_irt_mgl_signal_sync_point_valid = true;
  }
  return &g_irt_mgl_signal_sync_point;
}

}

MGLContext MGLCreateContext(MGLOpenGLAPIVersion version,
                            MojoHandle command_buffer_handle,
                            MGLContext share_group,
                            MGLContextLostCallback lost_callback,
                            void* lost_callback_closure,
                            const struct MojoAsyncWaiter* async_waiter) {
  struct nacl_irt_mgl* irt_mgl = GetIrtMGL();
  if (!irt_mgl)
    return MGL_NO_CONTEXT;
  return irt_mgl->MGLCreateContext(version,
                                   command_buffer_handle,
                                   share_group,
                                   lost_callback,
                                   lost_callback_closure,
                                   async_waiter);
}

void MGLDestroyContext(MGLContext context) {
  struct nacl_irt_mgl* irt_mgl = GetIrtMGL();
  if (irt_mgl)
    irt_mgl->MGLDestroyContext(context);
}

void MGLMakeCurrent(MGLContext context) {
  struct nacl_irt_mgl* irt_mgl = GetIrtMGL();
  if (irt_mgl)
    irt_mgl->MGLMakeCurrent(context);
}

MGLContext MGLGetCurrentContext(void) {
  struct nacl_irt_mgl* irt_mgl = GetIrtMGL();
  if (!irt_mgl)
    return MGL_NO_CONTEXT;
  return irt_mgl->MGLGetCurrentContext();
}

MGLMustCastToProperFunctionPointerType MGLGetProcAddress(const char* name) {
  struct nacl_irt_mgl* irt_mgl = GetIrtMGL();
  if (!irt_mgl)
    return nullptr;
  return irt_mgl->MGLGetProcAddress(name);
}

void MGLResizeSurface(uint32_t width, uint32_t height) {
  struct nacl_irt_mgl_onscreen* irt_mgl_onscreen = GetIrtMGLOnScreen();
  if (irt_mgl_onscreen)
    irt_mgl_onscreen->MGLResizeSurface(width, height);
}

void MGLSwapBuffers(void) {
  struct nacl_irt_mgl_onscreen* irt_mgl_onscreen = GetIrtMGLOnScreen();
  if (irt_mgl_onscreen)
    irt_mgl_onscreen->MGLSwapBuffers();
}

void MGLSignalSyncPoint(uint32_t sync_point,
                        MGLSignalSyncPointCallback callback,
                        void* closure) {
  struct nacl_irt_mgl_signal_sync_point *irt_mgl_signal_sync_point =
      GetIrtMGLSignalSyncPoint();
  if (irt_mgl_signal_sync_point)
    irt_mgl_signal_sync_point->MGLSignalSyncPoint(
        sync_point, callback, closure);
}
