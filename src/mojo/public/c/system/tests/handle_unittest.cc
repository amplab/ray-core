// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C handle API (the functions declared in
// mojo/public/c/system/handle.h). Note: The functionality of these APIs for
// specific types of handles are tested with the APIs for those types of
// handles.

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/result.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const MojoHandleRights kDefaultMessagePipeHandleRights =
    MOJO_HANDLE_RIGHT_TRANSFER | MOJO_HANDLE_RIGHT_READ |
    MOJO_HANDLE_RIGHT_WRITE | MOJO_HANDLE_RIGHT_GET_OPTIONS |
    MOJO_HANDLE_RIGHT_SET_OPTIONS;

TEST(HandleTest, InvalidHandle) {
  // MojoClose:
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(MOJO_HANDLE_INVALID));

  // MojoGetRights:
  MojoHandleRights rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoGetRights(MOJO_HANDLE_INVALID, &rights));

  // MojoReplaceHandleWithReducedRights:
  MojoHandle replacement_handle = MOJO_HANDLE_INVALID;
  EXPECT_EQ(
      MOJO_RESULT_INVALID_ARGUMENT,
      MojoReplaceHandleWithReducedRights(
          MOJO_HANDLE_INVALID, MOJO_HANDLE_RIGHT_NONE, &replacement_handle));

  // MojoDuplicateHandleWithReducedRights:
  MojoHandle new_handle = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoDuplicateHandleWithReducedRights(
                MOJO_HANDLE_INVALID, MOJO_HANDLE_RIGHT_DUPLICATE, &new_handle));
  EXPECT_EQ(MOJO_HANDLE_INVALID, new_handle);

  // MojoDuplicateHandle:
  new_handle = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoDuplicateHandle(MOJO_HANDLE_INVALID, &new_handle));
  EXPECT_EQ(MOJO_HANDLE_INVALID, new_handle);
}

// |MojoReplaceHandleWithReducedRights()| is not handle-type specific, so we'll
// test it here, even though it requires actually creating/using a specific
// handle type.
TEST(HandleTest, ReplaceHandleWithReducedRights) {
  MojoHandle h0 = MOJO_HANDLE_INVALID;
  MojoHandle h1 = MOJO_HANDLE_INVALID;
  // That |MojoCreateMessagePipe()| works correctly is checked in
  // |MessagePipeTest|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(nullptr, &h0, &h1));

  // Still check the rights on one of the handles, just to make sure that
  // |kDefaultMessagePipeHandleRights| stays in sync with reality.
  MojoHandleRights rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(h0, &rights));
  EXPECT_EQ(kDefaultMessagePipeHandleRights, rights);

  // First try replacing without reducing rights.
  MojoHandle h0r0 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoReplaceHandleWithReducedRights(
                                h0, MOJO_HANDLE_RIGHT_NONE, &h0r0));
  EXPECT_NE(h0r0, MOJO_HANDLE_INVALID);
  // Not guaranteed, but we depend on handle values not being reused eagerly.
  EXPECT_NE(h0r0, h0);
  EXPECT_NE(h0r0, h1);  // |h0r0| should definitely not be the same as |h1|.
  // |h0| should be dead, so this should fail.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(h0));

  // Check that the rights remain the same.
  rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(h0r0, &rights));
  EXPECT_EQ(kDefaultMessagePipeHandleRights, rights);

  // Make sure the replacement handle is still usable.
  char x = 'x';
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteMessage(h0r0, &x, 1u, nullptr, 0,
                                             MOJO_WRITE_MESSAGE_FLAG_NONE));

  // Try replacing, but removing a couple of rights.
  MojoHandle h0r1 = MOJO_HANDLE_INVALID;
  constexpr MojoHandleRights kRightsToRemove =
      MOJO_HANDLE_RIGHT_TRANSFER | MOJO_HANDLE_RIGHT_WRITE;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoReplaceHandleWithReducedRights(h0r0, kRightsToRemove, &h0r1));
  EXPECT_NE(h0r1, MOJO_HANDLE_INVALID);
  // Not guaranteed, but we depend on handle values not being reused eagerly.
  EXPECT_NE(h0r1, h0r0);
  EXPECT_NE(h0r1, h1);  // |h0r1| should definitely not be the same as |h1|.
  // |h0r0| should be dead, so this should fail.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(h0r0));

  // Check that |h0r1| has the expected rights.
  rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(h0r1, &rights));
  EXPECT_EQ(kDefaultMessagePipeHandleRights & ~kRightsToRemove, rights);

  // Make sure that the rights are actually correctly enforced.
  EXPECT_EQ(
      MOJO_RESULT_PERMISSION_DENIED,
      MojoWriteMessage(h0r1, &x, 1u, nullptr, 0, MOJO_WRITE_MESSAGE_FLAG_NONE));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h0r1));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h1));
}

}  // namespace
