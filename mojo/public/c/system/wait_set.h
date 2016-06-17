// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains functions for waiting on multiple handles using wait sets.
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_SYSTEM_WAIT_SET_H_
#define MOJO_PUBLIC_C_SYSTEM_WAIT_SET_H_

#include <stdint.h>

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/macros.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/c/system/time.h"

// |MojoCreateWaitSetOptions|: Used to specify creation parameters for a wait
// set to |MojoCreateWaitSet()|.
//   |uint32_t struct_size|: Set to the size of the |MojoCreateWaitSetOptions|
//       struct. (Used to allow for future extensions.)
//   |MojoCreateWaitSetOptionsFlags flags|: Reserved for future use
//       |MOJO_CREATE_WAIT_SET_OPTIONS_FLAGS_NONE|: No flags, default mode.

typedef uint32_t MojoCreateWaitSetOptionsFlags;

#define MOJO_CREATE_WAIT_SET_OPTIONS_FLAG_NONE \
  ((MojoCreateWaitSetOptionsFlags)0)

struct MOJO_ALIGNAS(8) MojoCreateWaitSetOptions {
  uint32_t struct_size;
  MojoCreateWaitSetOptionsFlags flags;
};
MOJO_STATIC_ASSERT(sizeof(struct MojoCreateWaitSetOptions) == 8,
                   "MojoCreateWaitSetOptions has wrong size");

// |MojoWaitSetAddOptions|: Used to specify parameters in adding an entry to a
// wait set with |MojoWaitSetAdd()|.
//   |uint32_t struct_size|: Set to the size of the |MojoWaitSetAddOptions|
//       struct. (Used to allow for future extensions.)
//   |MojoWaitSetAddOptionsFlags flags|: Reserved for future use.
//       |MOJO_WAIT_SET_ADD_OPTIONS_FLAGS_NONE|: No flags, default mode.

typedef uint32_t MojoWaitSetAddOptionsFlags;

#define MOJO_WAIT_SET_ADD_OPTIONS_FLAG_NONE ((MojoWaitSetAddOptionsFlags)0)

struct MOJO_ALIGNAS(8) MojoWaitSetAddOptions {
  uint32_t struct_size;
  MojoWaitSetAddOptionsFlags flags;
};
MOJO_STATIC_ASSERT(sizeof(struct MojoWaitSetAddOptions) == 8,
                   "MojoWaitSetAddOptions has wrong size");

// |MojoWaitSetResult|: Returned by |MojoWaitSetWait()| to indicate the state of
// entries. See |MojoWaitSetWait()| for the values of these fields.

struct MOJO_ALIGNAS(8) MojoWaitSetResult {
  uint64_t cookie;
  MojoResult wait_result;
  uint32_t reserved;
  MojoHandleSignals satisfied_signals;
  MojoHandleSignals satisfiable_signals;
};
MOJO_STATIC_ASSERT(sizeof(struct MojoWaitSetResult) == 24,
                   "MojoWaitSetResult has wrong size");

MOJO_BEGIN_EXTERN_C

// |MojoCreateWaitSet()|: Creates a new wait set.
//
// A wait set is an object that can be used to wait for a set of handles to
// satisfy some set of signals simultaneously.
//
// If |options| is null, the default options will be used.
//
// Returns:
//   |MOJO_RESULT_OK| if a wait set was successfully created. On success,
//       |*handle| will be the handle of the wait set.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |options| is non null and |*options| is invalid).
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if a process/system/quota/etc. limit has
//       been reached.
MojoResult MojoCreateWaitSet(const struct MojoCreateWaitSetOptions*
                                 MOJO_RESTRICT options,  // Optional in.
                             MojoHandle* handle);        // Out.

// |MojoWaitSetAdd()|: Adds an entry to watch for to the wait set specified by
// |wait_set_handle| (which must have the |MOJO_HANDLE_RIGHT_WRITE| right).
//
// An entry in a wait set is composed of a handle, a signal set, and a
// caller-specified cookie value. The cookie value must be unique across all
// entries in a particular wait set but is otherwise opaque and can be any value
// that is useful to the caller. If |options| is null the default options will
// be used.
//
// In all failure cases the wait set is unchanged.
//
// Returns:
//   |MOJO_RESULT_OK| if the handle was added to the wait set.
//   |MOJO_RESULT_INVALID_ARGUMENT| if |wait_set_handle| or |handle| do not
//       refer to valid handles, |wait_set_handle| is not a handle to a wait
//       set, or |options| is not null and |*options| is not a valid options
//       structure.
//   |MOJO_RESULT_ALREADY_EXISTS| if there is already an entry in the wait set
//       with the same |cookie| value.
//   |MOJO_RESULT_BUSY| if |wait_set_handle| or |handle| are currently in use in
//       some transaction.
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if the handle could not be added due to
//       hitting a system or quota limitation.
MojoResult MojoWaitSetAdd(const struct MojoWaitSetAddOptions* MOJO_RESTRICT
                              options,                 // Optional in.
                          MojoHandle wait_set_handle,  // In.
                          MojoHandle handle,           // In.
                          MojoHandleSignals signals,   // In.
                          uint64_t cookie);            // In.

// |MojoWaitSetRemove()|: Removes an entry from the wait set specified by
// |wait_set_handle| (which must have the |MOJO_HANDLE_RIGHT_WRITE| right).
//
// Returns:
//   |MOJO_RESULT_OK| if the entry was successfully removed.
//   |MOJO_RESULT_INVALID_ARGUMENT| if |wait_set_handle| does not refer to a
//       valid wait set.
//   |MOJO_RESULT_NOT_FOUND| if |cookie| does not identify an entry within the
//       wait set.
MojoResult MojoWaitSetRemove(MojoHandle wait_set_handle,  // In.
                             uint64_t cookie);            // In.

// |MojoWaitSetWait()|: Waits on all entries in the wait set specified by
// |wait_set_handle| (which must have the |MOJO_HANDLE_RIGHT_READ| right) for at
// least one of the following:
//   - At least one entry's handle satisfies a signal in that entry's signal
//     set.
//   - At least one entry's handle can never satisfy a signal in that entry's
//     signal set.
//   - |deadline| expires.
//   - The wait set is closed.
//
// On success, sets |*max_results| to the total number of possible results at
// the time of the call. Also returns information for up to |*num_results|
// entries in |*results| and sets |*num_results| to the number of entries the
// system populated. In particular, |results[0]|, ..., |results[*num_results-1]|
// will be populated as follows:
//   - |cookie| is the cookie value specified when the entry was added
//   - |wait_result| is set to one of the following:
//     - |MOJO_RESULT_OK| if the handle referred to by the entry satisfies one
//         or more of the signals in the entry
//     - |MOJO_RESULT_CANCELLED| if the handle referred to by the entry was
//         closed
//     - |MOJO_RESULT_BUSY| if the handle referred to by the entry is currently
//         in use in some transaction
//     - |MOJO_RESULT_FAILED_PRECONDITION| if it becomes impossible that the
//         handle referred to by the entry will ever satisfy any of entry's
//         signals
//   - |reserved| is set to 0
//
//   When the |wait_result| is |MOJO_RESULT_OK| or
//   |MOJO_RESULT_FAILED_PRECONDITION| the |satisfied_signals| and
//   |satisfiable_signals| entries are set to the signals the handle currently
//   satisfies and could possibly satisfy. When the |wait_result| is any other
//   value the |satisfied_signals| and |satisfiable_signals| entries are set to
//   |MOJO_HANDLE_SIGNALS_NONE|.
//
// On any result other than |MOJO_RESULT_OK|, |*num_results|, |*results| and
// |*max_results| are not modified.
//
// Returns:
//   |MOJO_RESULT_OK| if one or more entries in the wait set become satisfied or
//       unsatisfiable.
//   |MOJO_RESULT_INVALID_ARGUMENT| if |wait_set_handle| does not refer to a
//       valid wait set handle.
//   |MOJO_RESULT_CANCELLED| if |wait_set_handle| is closed during the wait.
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if a system/quota/etc. limit was reached.
//   |MOJO_RESULT_BUSY| if |wait_set_handle| is in use in some transaction. Note
//       that waiting on a wait set handle does not count as a transaction. It
//       is valid to call |MojoWaitSetWait()| on the same wait set handle
//       concurrently from different threads.
//   |MOJO_RESULT_DEADLINE_EXCEEDED| if the deadline is passed without any
//       entries in the wait set becoming satisfied or unsatisfiable.
MojoResult MojoWaitSetWait(
    MojoHandle wait_set_handle,                       // In.
    MojoDeadline deadline,                            // In.
    uint32_t* MOJO_RESTRICT num_results,              // In/out.
    struct MojoWaitSetResult* MOJO_RESTRICT results,  // Out.
    uint32_t* MOJO_RESTRICT max_results);             // Optional out.

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_SYSTEM_WAIT_SET_H_
