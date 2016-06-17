// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_PLATFORM_NATIVE_PLATFORM_HANDLE_PRIVATE_H_
#define MOJO_PUBLIC_PLATFORM_NATIVE_PLATFORM_HANDLE_PRIVATE_H_

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"

// |MojoPlatformHandle|: Type for "platform handles", i.e., the underlying OS's
// handles. Currently this is always just a Unix file descriptor.

typedef int MojoPlatformHandle;

#ifdef __cplusplus
extern "C" {
#endif

// |MojoCreatePlatformHandleWrapper()|: Creates a |MojoHandle| that wraps (and
// takes ownership of) the platform handle |platform_handle|, which must be
// valid.
//
// On success, |*platform_handle_wrapper_handle| will be set to the wrapper
// handle. It will have (at least) the |MOJO_HANDLE_RIGHT_TRANSFER|,
// |MOJO_HANDLE_RIGHT_READ|, and |MOJO_HANDLE_RIGHT_WRITE| rights. Warning: No
// validation of |platform_handle| is done. (TODO(vtl): This has poor/annoying
// implications, since we may detect this when we transfer the wrapper handle.)
//
// Warning: On failure, this will still take ownership of |platform_handle|
// (which just means that |platform_handle| will be closed).
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if a process/system/quota/etc. limit has
//       been reached (e.g., if the maximum number of handles was exceeded).
MojoResult MojoCreatePlatformHandleWrapper(
    MojoPlatformHandle platform_handle,
    MojoHandle* platform_handle_wrapper_handle);

// |MojoExtractPlatformHandle()|: Extracts the wrapped platform handle from
// |platform_handle_wrapper_handle| (which must have both the
// |MOJO_HANDLE_RIGHT_READ| and |MOJO_HANDLE_RIGHT_WRITE| rights).
//
// On success, |*platform_handle| will be set to the wrapped platform handle and
// ownership of the wrapped platform handle will be passed to the caller (i.e.,
// closing |platform_handle_wrapper_handle| will no longer close the platform
// handle).
//
// Warnings:
//   - Even though |platform_handle_wrapper_handle| is then basically useless
//     (it no longer "contains" a platform handle), it must still be closed as
//     usual.
//   - If the wrapped platform handle has already been extracted from
//     |platform_handle_wrapper_handle|, then this will still succeed, but
//     |*platform_handle| will be set to -1.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |platform_handle_wrapper_handle| is not a valid wrapper handle).
//   |MOJO_RESULT_PERMISSION_DENIED| if |platform_handle_wrapper_handle| does
//       not have the both the |MOJO_HANDLE_RIGHT_READ| and
//       |MOJO_HANDLE_RIGHT_WRITE| rights.
MojoResult MojoExtractPlatformHandle(MojoHandle platform_handle_wrapper_handle,
                                     MojoPlatformHandle* platform_handle);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // MOJO_PUBLIC_PLATFORM_NATIVE_PLATFORM_HANDLE_PRIVATE_H_
