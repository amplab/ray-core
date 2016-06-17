// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_PLATFORM_NATIVE_MGL_ECHO_THUNKS_H_
#define MOJO_PUBLIC_PLATFORM_NATIVE_MGL_ECHO_THUNKS_H_

#include <stddef.h>

#include "mojo/public/c/gpu/MGL/mgl_echo.h"

// Structure used to bind the MGL echo interface DSO to those of the embedder.
//
// This is the ABI between the embedder and the DSO. It can only have new
// functions added to the end. No other changes are supported.
#pragma pack(push, 8)
struct MGLEchoThunks {
  size_t size;  // Should be set to sizeof(MGLEchoThunks).

  void (*MGLEcho)(MGLEchoCallback callback, void* lost_callback_closure);
};
#pragma pack(pop)

#ifdef __cplusplus
// Intended to be called from the embedder. Returns an object initialized to
// contain pointers to each of the embedder's MGLEchoThunks
// functions.
inline struct MGLEchoThunks MojoMakeMGLEchoThunks() {
  struct MGLEchoThunks mgl_thunks = {
      sizeof(struct MGLEchoThunks), MGLEcho,
  };

  return mgl_thunks;
}
#endif  // __cplusplus

// Use this type for the function found by dynamically discovering it in
// a DSO linked with mojo_system. For example:
// MojoSetMGLEchoThunksFn mojo_set_gles2_thunks_fn =
//     reinterpret_cast<MojoSetMGLEchoThunksFn>(
//         app_library.GetFunctionPointer("MojoSetMGLEchoThunks"));
// The expected size of |mgl_thunks| is returned.
// The contents of |mgl_thunks| are copied.
typedef size_t (*MojoSetMGLEchoThunksFn)(
    const struct MGLEchoThunks* mgl_echo_thunks);

#endif  // MOJO_PUBLIC_PLATFORM_NATIVE_MGL_ECHO_THUNKS_H_
