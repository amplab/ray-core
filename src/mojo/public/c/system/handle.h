// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains Mojo system handle-related declarations/definitions.
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_SYSTEM_HANDLE_H_
#define MOJO_PUBLIC_C_SYSTEM_HANDLE_H_

#include <stdint.h>

#include "mojo/public/c/system/macros.h"
#include "mojo/public/c/system/result.h"

// |MojoHandle|: Handles to Mojo objects.
//   |MOJO_HANDLE_INVALID| - A value that is never a valid handle.

typedef uint32_t MojoHandle;

#define MOJO_HANDLE_INVALID ((MojoHandle)0)

// |MojoHandleRights|: Rights ("permissions") for handles. Each handle has a
// bitset of rights flags, which subsequently only be reduced. See each function
// for details about the right(s) required (some, like |MojoClose()|, require no
// rights).
//   |MOJO_HANDLE_RIGHT_NONE| - No rights.
//   |MOJO_HANDLE_RIGHT_DUPLICATE| - Right to duplicate a handle.
//   |MOJO_HANDLE_RIGHT_TRANSFER| - Right to transfer a handle (e.g., include in
//       a message).
//   |MOJO_HANDLE_RIGHT_READ| - Right to "read" from the handle (e.g., read a
//       message).
//   |MOJO_HANDLE_RIGHT_WRITE| - Right to "write" to the handle (e.g., write a
//       message).
//   |MOJO_HANDLE_RIGHT_GET_OPTIONS| - Right to get a handle's options.
//   |MOJO_HANDLE_RIGHT_SET_OPTIONS| - Right to set a handle's options.
//   |MOJO_HANDLE_RIGHT_MAP_READABLE| - Right to "map" a (e.g., buffer) handle
//       as readable memory.
//   |MOJO_HANDLE_RIGHT_MAP_WRITABLE| - Right to "map" a (e.g., buffer) handle
//       as writable memory.
//   |MOJO_HANDLE_RIGHT_MAP_EXECUTABLE| - Right to "map" a (e.g., buffer) handle
//       as executable memory.
//
// TODO(vtl): Add rights support/checking to existing handle types.

typedef uint32_t MojoHandleRights;

#define MOJO_HANDLE_RIGHT_NONE ((MojoHandleRights)0)
#define MOJO_HANDLE_RIGHT_DUPLICATE ((MojoHandleRights)1 << 0)
#define MOJO_HANDLE_RIGHT_TRANSFER ((MojoHandleRights)1 << 1)
#define MOJO_HANDLE_RIGHT_READ ((MojoHandleRights)1 << 2)
#define MOJO_HANDLE_RIGHT_WRITE ((MojoHandleRights)1 << 3)
#define MOJO_HANDLE_RIGHT_GET_OPTIONS ((MojoHandleRights)1 << 4)
#define MOJO_HANDLE_RIGHT_SET_OPTIONS ((MojoHandleRights)1 << 5)
#define MOJO_HANDLE_RIGHT_MAP_READABLE ((MojoHandleRights)1 << 6)
#define MOJO_HANDLE_RIGHT_MAP_WRITABLE ((MojoHandleRights)1 << 7)
#define MOJO_HANDLE_RIGHT_MAP_EXECUTABLE ((MojoHandleRights)1 << 8)

// |MojoHandleSignals|: Used to specify signals that can be waited on for a
// handle (and which can be triggered), e.g., the ability to read or write to
// the handle.
//   |MOJO_HANDLE_SIGNAL_NONE| - No flags. |MojoWait()|, etc. will return
//       |MOJO_RESULT_FAILED_PRECONDITION| if you attempt to wait on this.
//   |MOJO_HANDLE_SIGNAL_READABLE| - Can read (e.g., a message) from the handle.
//   |MOJO_HANDLE_SIGNAL_WRITABLE| - Can write (e.g., a message) to the handle.
//   |MOJO_HANDLE_SIGNAL_PEER_CLOSED| - The peer handle is closed.
//   |MOJO_HANDLE_SIGNAL_READ_THRESHOLD| - Can read a certain amount of data
//       from the handle (e.g., a data pipe consumer; see
//       |MojoDataPipeConsumerOptions|).

typedef uint32_t MojoHandleSignals;

#define MOJO_HANDLE_SIGNAL_NONE ((MojoHandleSignals)0)
#define MOJO_HANDLE_SIGNAL_READABLE ((MojoHandleSignals)1 << 0)
#define MOJO_HANDLE_SIGNAL_WRITABLE ((MojoHandleSignals)1 << 1)
#define MOJO_HANDLE_SIGNAL_PEER_CLOSED ((MojoHandleSignals)1 << 2)
#define MOJO_HANDLE_SIGNAL_READ_THRESHOLD ((MojoHandleSignals)1 << 3)
#define MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD ((MojoHandleSignals)1 << 4)

// |MojoHandleSignalsState|: Returned by wait functions to indicate the
// signaling state of handles. Members are as follows:
//   - |satisfied signals|: Bitmask of signals that were satisfied at some time
//         before the call returned.
//   - |satisfiable signals|: These are the signals that are possible to
//         satisfy. For example, if the return value was
//         |MOJO_RESULT_FAILED_PRECONDITION|, you can use this field to
//         determine which, if any, of the signals can still be satisfied.
// Note: This struct is not extensible (and only has 32-bit quantities), so it's
// 32-bit-aligned.

MOJO_STATIC_ASSERT(MOJO_ALIGNOF(uint32_t) == 4, "uint32_t has weird alignment");
struct MOJO_ALIGNAS(4) MojoHandleSignalsState {
  MojoHandleSignals satisfied_signals;
  MojoHandleSignals satisfiable_signals;
};
MOJO_STATIC_ASSERT(sizeof(struct MojoHandleSignalsState) == 8,
                   "MojoHandleSignalsState has wrong size");

MOJO_BEGIN_EXTERN_C

// |MojoClose()|: Closes the given |handle|.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if |handle| is not a valid handle.
//   |MOJO_RESULT_BUSY| if |handle| is currently in use in some transaction
//       (that, e.g., may result in it being invalidated, such as being sent in
//       a message).
//
// Concurrent operations on |handle| may succeed (or fail as usual) if they
// happen before the close, be cancelled with result |MOJO_RESULT_CANCELLED| if
// they properly overlap (this is likely the case with |MojoWait()|, etc.), or
// fail with |MOJO_RESULT_INVALID_ARGUMENT| if they happen after.
MojoResult MojoClose(MojoHandle handle);  // In.

// |MojoGetRights()|: Gets the given |handle|'s rights.
//
// On success, |*rights| (|rights| must be non-null) will be set to |handle|'s
// rights.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if |handle| is not a valid handle.
//   |MOJO_RESULT_BUSY| if |handle| is currently in use in some transaction
//       (that, e.g., may result in it being invalidated, such as being sent in
//       a message).
MojoResult MojoGetRights(MojoHandle handle, MojoHandleRights* rights);  // Out.

// |MojoReplaceHandleWithReducedRights()|: Replaces |handle| with an equivalent
// one with reduced rights.
//
// On success, |*replacement_handle| will be a handle that is equivalent to
// |handle| (before the call), but with:
//
//   replacement handle rights = current rights & ~rights_to_remove.
//
// |handle| will be invalidated and any ongoing two-phase operations (e.g., for
// data pipes) on |handle| will be aborted.
//
// On failure, |handle| will remain valid and unchanged (with any ongoing
// two-phase operations undisturbed) and |*replacement_handle| will not be set.
//
// Note that it is not an error to "remove" rights that the handle does not
// (currently) possess.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if |handle| is not a valid handle.
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if a process/system/quota/etc. limit has
//       been reached.
//   |MOJO_RESULT_BUSY| if |handle| is currently in use in some transaction
//       (that, e.g., may result in it being invalidated, such as being sent in
//       a message).
MojoResult MojoReplaceHandleWithReducedRights(
    MojoHandle handle,
    MojoHandleRights rights_to_remove,
    MojoHandle* replacement_handle);  // Out.

// |MojoDuplicateHandleWithReducedRights()|: Duplicates |handle| to a new handle
// with reduced rights. This requires |handle| to have the
// |MOJO_HANDLE_RIGHT_DUPLICATE| (note that some handle types may never have
// this right).
//
// The rights for the new handle are determined as in
// |MojoReplaceHandleWithReducedRights()|. That is, on success:
//
//   new handle rights = original handle rights & ~rights_to_remove.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if |handle| is not a valid handle.
//   |MOJO_RESULT_PERMISSION_DENIED| if |handle| does not have the
//       |MOJO_HANDLE_RIGHT_DUPLICATE| right.
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if a process/system/quota/etc. limit has
//       been reached.
//   |MOJO_RESULT_BUSY| if |handle| is currently in use in some transaction
//       (that, e.g., may result in it being invalidated, such as being sent in
//       a message).
MojoResult MojoDuplicateHandleWithReducedRights(
    MojoHandle handle,
    MojoHandleRights rights_to_remove,
    MojoHandle* new_handle);  // Out.

// |MojoDuplicateHandle()|: Duplicates |handle| to a new handle with equivalent
// rights. This requires |handle| to have the |MOJO_HANDLE_RIGHT_DUPLICATE|.
// This is equivalent to |MojoDuplicateHandleWithReducedRights(handle,
// MOJO_HANDLE_RIGHT_NONE, new_handle)|.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if |handle| is not a valid handle.
//   |MOJO_RESULT_PERMISSION_DENIED| if |handle| does not have the
//       |MOJO_HANDLE_RIGHT_DUPLICATE| right.
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if a process/system/quota/etc. limit has
//       been reached.
//   |MOJO_RESULT_BUSY| if |handle| is currently in use in some transaction
//       (that, e.g., may result in it being invalidated, such as being sent in
//       a message).
MojoResult MojoDuplicateHandle(MojoHandle handle,
                               MojoHandle* new_handle);  // Out.

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_SYSTEM_HANDLE_H_
