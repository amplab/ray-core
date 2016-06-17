// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C buffer API (the functions declared in
// mojo/public/c/system/buffer.h).

#include "mojo/public/c/system/buffer.h"
#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const MojoHandleRights kDefaultSharedBufferHandleRights =
    MOJO_HANDLE_RIGHT_DUPLICATE | MOJO_HANDLE_RIGHT_TRANSFER |
    MOJO_HANDLE_RIGHT_GET_OPTIONS | MOJO_HANDLE_RIGHT_SET_OPTIONS |
    MOJO_HANDLE_RIGHT_MAP_READABLE | MOJO_HANDLE_RIGHT_MAP_WRITABLE |
    MOJO_HANDLE_RIGHT_MAP_EXECUTABLE;

// The only handle that's guaranteed to be invalid is |MOJO_HANDLE_INVALID|.
// Tests that everything that takes a handle properly recognizes it.
TEST(BufferTest, InvalidHandle) {
  MojoHandle out_handle = MOJO_HANDLE_INVALID;
  EXPECT_EQ(
      MOJO_RESULT_INVALID_ARGUMENT,
      MojoDuplicateBufferHandle(MOJO_HANDLE_INVALID, nullptr, &out_handle));
  MojoBufferInformation buffer_info = {};
  EXPECT_EQ(
      MOJO_RESULT_INVALID_ARGUMENT,
      MojoGetBufferInformation(MOJO_HANDLE_INVALID, &buffer_info,
                               static_cast<uint32_t>(sizeof(buffer_info))));
  void* write_pointer = nullptr;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoMapBuffer(MOJO_HANDLE_INVALID, 0u, 1u, &write_pointer,
                          MOJO_MAP_BUFFER_FLAG_NONE));
  // This isn't an "invalid handle" test, but we'll throw it in here anyway
  // (since it involves a look-up).
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoUnmapBuffer(nullptr));
}

// TODO(ncbray): enable this test once NaCl supports the corresponding APIs.
#ifdef __native_client__
#define MAYBE_Basic DISABLED_Basic
#else
#define MAYBE_Basic Basic
#endif
TEST(BufferTest, MAYBE_Basic) {
  // Create a shared buffer (|h0|).
  MojoHandle h0 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateSharedBuffer(nullptr, 100, &h0));
  EXPECT_NE(h0, MOJO_HANDLE_INVALID);

  // The handle should have the correct rights.
  MojoHandleRights rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(h0, &rights));
  EXPECT_EQ(kDefaultSharedBufferHandleRights, rights);

  // Check information about the buffer from |h0|.
  MojoBufferInformation info = {};
  static const uint32_t kInfoSize = static_cast<uint32_t>(sizeof(info));
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetBufferInformation(h0, &info, kInfoSize));
  EXPECT_EQ(kInfoSize, info.struct_size);
  EXPECT_EQ(MOJO_BUFFER_INFORMATION_FLAG_NONE, info.flags);
  EXPECT_EQ(100u, info.num_bytes);

  // Map everything.
  void* pointer = nullptr;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoMapBuffer(h0, 0, 100, &pointer, MOJO_MAP_BUFFER_FLAG_NONE));
  ASSERT_TRUE(pointer);
  static_cast<char*>(pointer)[50] = 'x';

  // Duplicate |h0| to |h1|.
  MojoHandle h1 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoDuplicateHandle(h0, &h1));
  EXPECT_NE(h1, MOJO_HANDLE_INVALID);
  EXPECT_NE(h1, h0);

  // The new handle should have the correct rights.
  rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(h1, &rights));
  EXPECT_EQ(kDefaultSharedBufferHandleRights, rights);

  // Check information about the buffer from |h1|.
  info = MojoBufferInformation();
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetBufferInformation(h1, &info, kInfoSize));
  EXPECT_EQ(kInfoSize, info.struct_size);
  EXPECT_EQ(MOJO_BUFFER_INFORMATION_FLAG_NONE, info.flags);
  EXPECT_EQ(100u, info.num_bytes);

  // Close |h0|.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h0));

  // The mapping should still be good.
  static_cast<char*>(pointer)[51] = 'y';

  // Unmap it.
  EXPECT_EQ(MOJO_RESULT_OK, MojoUnmapBuffer(pointer));

  // Map half of |h1|.
  pointer = nullptr;
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoMapBuffer(h1, 50, 50, &pointer, MOJO_MAP_BUFFER_FLAG_NONE));
  ASSERT_TRUE(pointer);

  // It should have what we wrote.
  EXPECT_EQ('x', static_cast<char*>(pointer)[0]);
  EXPECT_EQ('y', static_cast<char*>(pointer)[1]);

  // Unmap it.
  EXPECT_EQ(MOJO_RESULT_OK, MojoUnmapBuffer(pointer));

  // Test duplication with reduced rights.
  MojoHandle h2 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_OK, MojoDuplicateHandleWithReducedRights(
                                h1, MOJO_HANDLE_RIGHT_DUPLICATE, &h2));
  EXPECT_NE(h2, MOJO_HANDLE_INVALID);
  EXPECT_NE(h2, h1);
  // |h2| should have the correct rights.
  rights = MOJO_HANDLE_RIGHT_NONE;
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetRights(h2, &rights));
  EXPECT_EQ(kDefaultSharedBufferHandleRights & ~MOJO_HANDLE_RIGHT_DUPLICATE,
            rights);
  // Trying to duplicate |h2| should fail.
  MojoHandle h3 = MOJO_HANDLE_INVALID;
  EXPECT_EQ(MOJO_RESULT_PERMISSION_DENIED, MojoDuplicateHandle(h2, &h3));
  EXPECT_EQ(MOJO_HANDLE_INVALID, h3);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h1));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h2));
}

// TODO(vtl): Add multi-threaded tests.

}  // namespace
