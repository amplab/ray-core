// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains functions for waiting on handles (directly).
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_SYSTEM_WAIT_H_
#define MOJO_PUBLIC_C_SYSTEM_WAIT_H_

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/macros.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/c/system/time.h"

MOJO_BEGIN_EXTERN_C

// |MojoWait()|: Waits on the given handle until one of the following happens:
//   - A signal indicated by |signals| is satisfied.
//   - It becomes known that no signal indicated by |signals| will ever be
//     satisfied. (See the description of the |MOJO_RESULT_CANCELLED| and
//     |MOJO_RESULT_FAILED_PRECONDITION| return values below.)
//   - Until |deadline| has passed.
//
// If |deadline| is |MOJO_DEADLINE_INDEFINITE|, this will wait "forever" (until
// one of the other wait termination conditions is satisfied). If |deadline| is
// 0, this will return |MOJO_RESULT_DEADLINE_EXCEEDED| only if one of the other
// termination conditions (e.g., a signal is satisfied, or all signals are
// unsatisfiable) is not already satisfied.
//
// |signals_state| (optional): See documentation for |MojoHandleSignalsState|.
//
// Returns:
//   |MOJO_RESULT_OK| if some signal in |signals| was satisfied (or is already
//       satisfied).
//   |MOJO_RESULT_CANCELLED| if |handle| was closed (necessarily from another
//       thread) during the wait.
//   |MOJO_RESULT_INVALID_ARGUMENT| if |handle| is not a valid handle (e.g., if
//       it has already been closed). The |signals_state| value is unchanged.
//   |MOJO_RESULT_DEADLINE_EXCEEDED| if the deadline has passed without any of
//       the signals being satisfied.
//   |MOJO_RESULT_FAILED_PRECONDITION| if it becomes known that none of the
//       signals in |signals| can ever be satisfied (e.g., when waiting on one
//       end of a message pipe and the other end is closed), at least not
//       without performing some action on |handle| (see, e.g., |struct
//       MojoDataPipeConsumerOptions|).
//   |MOJO_RESULT_BUSY| if |handle| is currently in use in some transaction
//       (that, e.g., may result in it being invalidated, such as being sent in
//       a message).
//
// If there are multiple waiters (on different threads, obviously) waiting on
// the same handle and signal, and that signal becomes satisfied, all waiters
// will be awoken.
MojoResult MojoWait(
    MojoHandle handle,                              // In.
    MojoHandleSignals signals,                      // In.
    MojoDeadline deadline,                          // In.
    struct MojoHandleSignalsState* signals_state);  // Optional out.

// |MojoWaitMany()|: Waits on |handles[0]|, ..., |handles[num_handles-1]| until:
//   - (At least) one handle satisfies a signal indicated in its respective
//     |signals[0]|, ..., |signals[num_handles-1]|.
//   - It becomes known that no signal in some |signals[i]| will ever be
//     satisfied.
//   - |deadline| has passed.
//
// This means that |MojoWaitMany()| behaves as if |MojoWait()| were called on
// each handle/signals pair simultaneously, completing when the first
// |MojoWait()| would complete.
//
// See |MojoWait()| for more details about |deadline|.
//
// |result_index| (optional) is used to return the index of the handle that
//     caused the call to return. For example, the index |i| (from 0 to
//     |num_handles-1|) if |handle[i]| satisfies a signal from |signals[i]|. You
//     must manually initialize this to a suitable sentinel value (e.g. -1)
//     before you make this call because this value is not updated if there is
//     no specific handle that causes the function to return. Pass null if you
//     don't need this value to be returned.
//
// |signals_states| (optional) points to an array of size |num_handles| of
//     MojoHandleSignalsState. See |MojoHandleSignalsState| for more details
//     about the meaning of each array entry. This array is not an atomic
//     snapshot. The array will be updated if the function does not return
//     |MOJO_RESULT_INVALID_ARGUMENT| or |MOJO_RESULT_RESOURCE_EXHAUSTED|.
//
// Returns:
//   |MOJO_RESULT_CANCELLED| if some |handle[i]| was closed (necessarily from
//       another thread) during the wait.
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if there are too many handles. The
//       |signals_state| array is unchanged.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some |handle[i]| is not a valid handle
//       (e.g., if it is zero or if it has already been closed). The
//       |signals_state| array is unchanged.
//   |MOJO_RESULT_DEADLINE_EXCEEDED| if the deadline has passed without any of
//       handles satisfying any of its signals.
//   |MOJO_RESULT_FAILED_PRECONDITION| if it is or becomes impossible that SOME
//       |handle[i]| will ever satisfy any of the signals in |signals[i]|, at
//       least not without some action on |handle[i]| (see, e.g., |struct
//       MojoDataPipeConsumerOptions|).
//   |MOJO_RESULT_BUSY| if some |handle[i]| is currently in use in some
//       transaction (that, e.g., may result in it being invalidated, such as
//       being sent in a message).
MojoResult MojoWaitMany(const MojoHandle* MOJO_RESTRICT handles,         // In.
                        const MojoHandleSignals* MOJO_RESTRICT signals,  // In.
                        uint32_t num_handles,                            // In.
                        MojoDeadline deadline,                           // In.
                        uint32_t* MOJO_RESTRICT result_index,  // Optional out.
                        struct MojoHandleSignalsState* MOJO_RESTRICT
                            signals_states);  // Optional out.

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_SYSTEM_WAIT_H_
