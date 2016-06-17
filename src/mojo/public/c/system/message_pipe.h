// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains types/constants and functions specific to message pipes.
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_SYSTEM_MESSAGE_PIPE_H_
#define MOJO_PUBLIC_C_SYSTEM_MESSAGE_PIPE_H_

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/macros.h"
#include "mojo/public/c/system/result.h"

// |MojoCreateMessagePipeOptions|: Used to specify creation parameters for a
// message pipe to |MojoCreateMessagePipe()|.
//   |uint32_t struct_size|: Set to the size of the
//       |MojoCreateMessagePipeOptions| struct. (Used to allow for future
//       extensions.)
//   |MojoCreateMessagePipeOptionsFlags flags|: Reserved for future use.
//       |MOJO_CREATE_MESSAGE_PIPE_OPTIONS_FLAG_NONE|: No flags; default mode.

typedef uint32_t MojoCreateMessagePipeOptionsFlags;

#define MOJO_CREATE_MESSAGE_PIPE_OPTIONS_FLAG_NONE \
  ((MojoCreateMessagePipeOptionsFlags)0)

MOJO_STATIC_ASSERT(MOJO_ALIGNOF(int64_t) == 8, "int64_t has weird alignment");
struct MOJO_ALIGNAS(8) MojoCreateMessagePipeOptions {
  uint32_t struct_size;
  MojoCreateMessagePipeOptionsFlags flags;
};
MOJO_STATIC_ASSERT(sizeof(struct MojoCreateMessagePipeOptions) == 8,
                   "MojoCreateMessagePipeOptions has wrong size");

// |MojoWriteMessageFlags|: Used to specify different modes to
// |MojoWriteMessage()|.
//   |MOJO_WRITE_MESSAGE_FLAG_NONE| - No flags; default mode.

typedef uint32_t MojoWriteMessageFlags;

#define MOJO_WRITE_MESSAGE_FLAG_NONE ((MojoWriteMessageFlags)0)

// |MojoReadMessageFlags|: Used to specify different modes to
// |MojoReadMessage()|.
//   |MOJO_READ_MESSAGE_FLAG_NONE| - No flags; default mode.
//   |MOJO_READ_MESSAGE_FLAG_MAY_DISCARD| - If the message is unable to be read
//       for whatever reason (e.g., the caller-supplied buffer is too small),
//       discard the message (i.e., simply dequeue it).

typedef uint32_t MojoReadMessageFlags;

#define MOJO_READ_MESSAGE_FLAG_NONE ((MojoReadMessageFlags)0)
#define MOJO_READ_MESSAGE_FLAG_MAY_DISCARD ((MojoReadMessageFlags)1 << 0)

MOJO_BEGIN_EXTERN_C

// |MojoCreateMessagePipe()|: Creates a message pipe, which is a bidirectional
// communication channel for framed data (i.e., messages). Messages can contain
// plain data and/or Mojo handles.
//
// |options| may be set to null for a message pipe with the default options.
//
// On success, |*message_pipe_handle0| and |*message_pipe_handle1| are set to
// handles for the two endpoints (ports) for the message pipe. Both handles have
// (at least) the following rights: |MOJO_HANDLE_RIGHT_TRANSFER|,
// |MOJO_HANDLE_RIGHT_READ|, |MOJO_HANDLE_RIGHT_WRITE|,
// |MOJO_HANDLE_RIGHT_GET_OPTIONS|, and |MOJO_HANDLE_RIGHT_SET_OPTIONS|.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |*options| is invalid).
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if a process/system/quota/etc. limit has
//       been reached.
MojoResult MojoCreateMessagePipe(
    const struct MojoCreateMessagePipeOptions* MOJO_RESTRICT
        options,                                      // Optional in.
    MojoHandle* MOJO_RESTRICT message_pipe_handle0,   // Out.
    MojoHandle* MOJO_RESTRICT message_pipe_handle1);  // Out.

// |MojoWriteMessage()|: Writes a message to the message pipe endpoint given by
// |message_pipe_handle| (which must have the |MOJO_HANDLE_RIGHT_WRITE| right),
// with message data specified by |bytes| of size |num_bytes| and attached
// handles specified by |handles| of count |num_handles|, and options specified
// by |flags|. If there is no message data, |bytes| may be null, in which case
// |num_bytes| must be zero. If there are no attached handles, |handles| may be
// null, in which case |num_handles| must be zero.
//
// If handles are attached, on success the handles will no longer be valid (the
// receiver will receive equivalent, but logically different, handles) and any
// ongoing two-phase operations (e.g., for data pipes) on them will be aborted.
// Handles to be sent should not be in simultaneous use (e.g., on another
// thread). On failure, any handles to be attached will remain valid (and
// two-phase operations will not be aborted).
//
// Returns:
//   |MOJO_RESULT_OK| on success (i.e., the message was enqueued).
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g., if
//       |message_pipe_handle| is not a valid handle, or some of the
//       requirements above are not satisfied).
//   |MOJO_RESULT_PERMISSION_DENIED| if |message_pipe_handle| does not have the
//       |MOJO_HANDLE_RIGHT_WRITE| right or if one of the handles to be sent
//       does not have the |MOJO_HANDLE_RIGHT_TRANSFER| right.
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if some system limit has been reached, or
//       the number of handles to send is too large (TODO(vtl): reconsider the
//       latter case).
//   |MOJO_RESULT_FAILED_PRECONDITION| if the other endpoint has been closed.
//       Note that closing an endpoint is not necessarily synchronous (e.g.,
//       across processes), so this function may succeed even if the other
//       endpoint has been closed (in which case the message would be dropped).
//   |MOJO_RESULT_UNIMPLEMENTED| if an unsupported flag was set in |*options|.
//   |MOJO_RESULT_BUSY| if |message_pipe_handle| is currently in use in some
//       transaction (that, e.g., may result in it being invalidated, such as
//       being sent in a message), or if some handle to be sent is currently in
//       use.
//
// Note: |MOJO_RESULT_BUSY| is generally "preferred" over
// |MOJO_RESULT_PERMISSION_DENIED|. E.g., if a handle to be sent both is busy
// and does not have the transfer right, then the result will be "busy".
//
// TODO(vtl): We'll also report |MOJO_RESULT_BUSY| if a (data pipe
// producer/consumer) handle to be sent is in a two-phase write/read). But
// should we? (For comparison, there's no such provision in |MojoClose()|.)
// https://github.com/domokit/mojo/issues/782
//
// TODO(vtl): Add a notion of capacity for message pipes, and return
// |MOJO_RESULT_SHOULD_WAIT| if the message pipe is full.
MojoResult MojoWriteMessage(MojoHandle message_pipe_handle,  // In.
                            const void* bytes,               // Optional in.
                            uint32_t num_bytes,              // In.
                            const MojoHandle* handles,       // Optional in.
                            uint32_t num_handles,            // In.
                            MojoWriteMessageFlags flags);    // In.

// |MojoReadMessage()|: Reads the next message from the message pipe endpoint
// given by |message_pipe_handle| (which must have the |MOJO_HANDLE_RIGHT_READ|
// right) or indicates the size of the message if it cannot fit in the provided
// buffers. The message will be read in its entirety or not at all; if it is
// not, it will remain enqueued unless the |MOJO_READ_MESSAGE_FLAG_MAY_DISCARD|
// flag was passed. At most one message will be consumed from the queue, and the
// return value will indicate whether a message was successfully read.
//
// |num_bytes| and |num_handles| are optional in/out parameters that on input
// must be set to the sizes of the |bytes| and |handles| arrays, and on output
// will be set to the actual number of bytes or handles contained in the
// message (even if the message was not retrieved due to being too large).
// Either |num_bytes| or |num_handles| may be null if the message is not
// expected to contain the corresponding type of data, but such a call would
// fail with |MOJO_RESULT_RESOURCE_EXHAUSTED| if the message in fact did
// contain that type of data.
//
// |bytes| and |handles| will receive the contents of the message, if it is
// retrieved. Either or both may be null, in which case the corresponding size
// parameter(s) must also be set to zero or passed as null.
//
// Returns:
//   |MOJO_RESULT_OK| on success (i.e., a message was actually read).
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid.
//   |MOJO_RESULT_FAILED_PRECONDITION| if the other endpoint has been closed.
//   |MOJO_RESULT_PERMISSION_DENIED| if |message_pipe_handle| does not have the
//       |MOJO_HANDLE_RIGHT_READ| right.
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if the message was too large to fit in the
//       provided buffer(s). The message will have been left in the queue or
//       discarded, depending on flags.
//   |MOJO_RESULT_SHOULD_WAIT| if no message was available to be read.
//   |MOJO_RESULT_BUSY| if |message_pipe_handle| is currently in use in some
//       transaction (that, e.g., may result in it being invalidated, such as
//       being sent in a message).
//
// TODO(vtl): Reconsider the |MOJO_RESULT_RESOURCE_EXHAUSTED| error code; should
// distinguish this from the hitting-system-limits case.
MojoResult MojoReadMessage(
    MojoHandle message_pipe_handle,       // In.
    void* MOJO_RESTRICT bytes,            // Optional out.
    uint32_t* MOJO_RESTRICT num_bytes,    // Optional in/out.
    MojoHandle* MOJO_RESTRICT handles,    // Optional out.
    uint32_t* MOJO_RESTRICT num_handles,  // Optional in/out.
    MojoReadMessageFlags flags);          // In.

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_SYSTEM_MESSAGE_PIPE_H_
