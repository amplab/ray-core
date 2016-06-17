// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/platform/native/platform_handle_private.h"

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/environment/logging.h"
#include "mojo/public/cpp/system/macros.h"

namespace mojo {
namespace {

class PlatformHandlePrivateApplicationTest : public test::ApplicationTestBase {
 public:
  PlatformHandlePrivateApplicationTest() : ApplicationTestBase() {}
  ~PlatformHandlePrivateApplicationTest() override {}

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(PlatformHandlePrivateApplicationTest);
};

TEST_F(PlatformHandlePrivateApplicationTest, WrapAndUnwrapFileDescriptor) {
  MojoPlatformHandle original_handle = -1;
  MojoPlatformHandle unwrapped_handle = -1;
  MojoHandle wrapper = MOJO_HANDLE_INVALID;

  int pipe_fds[2] = {-1, -1};

  uint64_t write_buffer = 0xDEADBEEF;
  uint64_t read_buffer = 0;

  ASSERT_EQ(0, pipe(pipe_fds));

  // Pass second FD through wrapper.
  original_handle = pipe_fds[1];

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoCreatePlatformHandleWrapper(original_handle, &wrapper));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoExtractPlatformHandle(wrapper, &unwrapped_handle));

  // Write to wrapped/unwrapped FD.
  ssize_t bytes_written =
      write(unwrapped_handle, &write_buffer, sizeof(write_buffer));
  ASSERT_EQ(sizeof(write_buffer), static_cast<size_t>(bytes_written));

  // Read from other end of pipe.
  ssize_t bytes_read = read(pipe_fds[0], &read_buffer, sizeof(read_buffer));
  ASSERT_EQ(sizeof(read_buffer), static_cast<size_t>(bytes_read));

  EXPECT_EQ(bytes_read, bytes_written);
  EXPECT_EQ(write_buffer, read_buffer);

  EXPECT_EQ(0, close(pipe_fds[0]));
  EXPECT_EQ(0, close(unwrapped_handle));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(wrapper));
}

}  // namespace
}  // namespace mojo
