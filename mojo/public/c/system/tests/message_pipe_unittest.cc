// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C message pipe API (the functions declared in
// mojo/public/c/system/message_pipe.h).

#include "mojo/public/c/system/buffer.h"
#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/c/system/wait.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const MojoHandleRights kDefaultMessagePipeHandleRights =
    MOJO_HANDLE_RIGHT_TRANSFER | MOJO_HANDLE_RIGHT_READ |
    MOJO_HANDLE_RIGHT_WRITE | MOJO_HANDLE_RIGHT_GET_OPTIONS |
    MOJO_HANDLE_RIGHT_SET_OPTIONS;

TEST(MessagePipeTest, InvalidHandle) {
  char buffer[10] = {};
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWriteMessage(MOJO_HANDLE_INVALID, buffer, 0u, nullptr, 0u,
                             MOJO_WRITE_MESSAGE_FLAG_NONE));
  uint32_t buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoReadMessage(MOJO_HANDLE_INVALID, buffer, &buffer_size, nullptr,
                            nullptr, MOJO_READ_MESSAGE_FLAG_NONE));
}

TEST(MessagePipeTest, Basic) {
  MojoHandle h0 = MOJO_HANDLE_INVALID;
  MojoHandle h1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(nullptr, &h0, &h1));
  EXPECT_NE(h0, MOJO_HANDLE_INVALID);
  EXPECT_NE(h1, MOJO_HANDLE_INVALID);
  EXPECT_NE(h0, h1);

  // Both handles should have the correct rights.
  MojoHandleRights rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(h0, &rights));
  EXPECT_EQ(kDefaultMessagePipeHandleRights, rights);
  rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(h1, &rights));
  EXPECT_EQ(kDefaultMessagePipeHandleRights, rights);

  // Shouldn't be able to duplicate either handle (just test "with reduced
  // rights" on one, and without on the other).
  MojoHandle handle_denied = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_PERMISSION_DENIED,
            MojoDuplicateHandleWithReducedRights(
                h0, MOJO_HANDLE_RIGHT_DUPLICATE, &handle_denied));
  EXPECT_EQ(MOJO_HANDLE_INVALID, handle_denied);
  handle_denied = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_PERMISSION_DENIED,
            MojoDuplicateHandle(h1, &handle_denied));
  EXPECT_EQ(MOJO_HANDLE_INVALID, handle_denied);

  // Shouldn't be readable, we haven't written anything.
  MojoHandleSignalsState state;
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoWait(h0, MOJO_HANDLE_SIGNAL_READABLE, 0, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            state.satisfiable_signals);

  // Should be writable.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(h0, MOJO_HANDLE_SIGNAL_WRITABLE, 0, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            state.satisfiable_signals);

  // Last parameter is optional.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWait(h0, MOJO_HANDLE_SIGNAL_WRITABLE, 0, nullptr));

  // Try to read.
  char buffer[10] = {};
  uint32_t buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_SHOULD_WAIT,
            MojoReadMessage(h0, buffer, &buffer_size, nullptr, nullptr,
                            MOJO_READ_MESSAGE_FLAG_NONE));

  // Write to |h1|.
  static const char kHello[] = "hello";
  buffer_size = static_cast<uint32_t>(sizeof(kHello));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteMessage(h1, kHello, buffer_size, nullptr,
                                             0, MOJO_WRITE_MESSAGE_FLAG_NONE));

  // |h0| should be readable.
  MojoHandleSignals sig = MOJO_HANDLE_SIGNAL_READABLE;
  MojoHandleSignalsState states[1] = {};
  uint32_t result_index = 1;
  EXPECT_EQ(MOJO_RESULT_OK, MojoWaitMany(&h0, &sig, 1, MOJO_DEADLINE_INDEFINITE,
                                         &result_index, states));
  EXPECT_EQ(0u, result_index);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
            states[0].satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            states[0].satisfiable_signals);

  // Read from |h0|.
  buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReadMessage(h0, buffer, &buffer_size, nullptr, nullptr,
                            MOJO_READ_MESSAGE_FLAG_NONE));
  EXPECT_EQ(static_cast<uint32_t>(sizeof(kHello)), buffer_size);
  EXPECT_STREQ(kHello, buffer);

  // |h0| should no longer be readable.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            MojoWait(h0, MOJO_HANDLE_SIGNAL_READABLE, 10, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                MOJO_HANDLE_SIGNAL_PEER_CLOSED,
            state.satisfiable_signals);

  // Close |h0|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h0));

  // |h1| should no longer be readable or writable.
  EXPECT_EQ(
      MOJO_RESULT_FAILED_PRECONDITION,
      MojoWait(h1, MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
               1000, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfiable_signals);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h1));
}

TEST(MessagePipeTest, ChecksTransferRight) {
  MojoHandle h0 = MOJO_HANDLE_INVALID;
  MojoHandle h1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(nullptr, &h0, &h1));

  // Create a shared buffer (which is transferrable and duplicatable).
  MojoHandle h_transferrable = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoCreateSharedBuffer(nullptr, 100, &h_transferrable));

  // Make a non-transferrable duplicate handle.
  MojoHandle h_not_transferrable = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoDuplicateHandleWithReducedRights(
                                h_transferrable, MOJO_HANDLE_RIGHT_TRANSFER,
                                &h_not_transferrable));

  // |h_transferrable| can be transferred.
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteMessage(h0, nullptr, 0u, &h_transferrable,
                                             1u, MOJO_WRITE_MESSAGE_FLAG_NONE));

  // |h_not_transferrable| can be transferred.
  EXPECT_EQ(MOJO_RESULT_PERMISSION_DENIED,
            MojoWriteMessage(h0, nullptr, 0u, &h_not_transferrable, 1u,
                             MOJO_WRITE_MESSAGE_FLAG_NONE));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h0));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h1));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h_not_transferrable));
}

// TODO(vtl): Add multi-threaded tests.

}  // namespace
