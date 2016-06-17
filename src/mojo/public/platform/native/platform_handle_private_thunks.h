// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_PLATFORM_NATIVE_PLATFORM_HANDLE_PRIVATE_THUNKS_H_
#define MOJO_PUBLIC_PLATFORM_NATIVE_PLATFORM_HANDLE_PRIVATE_THUNKS_H_

#include <stddef.h>

#include "mojo/public/platform/native/platform_handle_private.h"

#pragma pack(push, 8)
struct MojoPlatformHandlePrivateThunks {
  size_t size;  // Should be set to sizeof(PlatformHandleThunks).
  MojoResult (*CreatePlatformHandleWrapper)(MojoPlatformHandle, MojoHandle*);
  MojoResult (*ExtractPlatformHandle)(MojoHandle, MojoPlatformHandle*);
};
#pragma pack(pop)

#ifdef __cplusplus
inline struct MojoPlatformHandlePrivateThunks
MojoMakePlatformHandlePrivateThunks() {
  struct MojoPlatformHandlePrivateThunks system_thunks = {
      sizeof(struct MojoPlatformHandlePrivateThunks),
      MojoCreatePlatformHandleWrapper, MojoExtractPlatformHandle};
  return system_thunks;
}
#endif  // __cplusplus

typedef size_t (*MojoSetPlatformHandlePrivateThunksFn)(
    const struct MojoPlatformHandlePrivateThunks* thunks);

#endif  // MOJO_PUBLIC_PLATFORM_NATIVE_PLATFORM_HANDLE_PRIVATE_THUNKS_H_
