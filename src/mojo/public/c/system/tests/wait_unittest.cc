// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C wait API (the functions declared in
// mojo/public/c/system/wait.h).

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/c/system/wait.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(WaitTest, InvalidHandle) {
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWait(MOJO_HANDLE_INVALID, ~MOJO_HANDLE_SIGNAL_NONE, 1000000u,
                     nullptr));

  const MojoHandle h = MOJO_HANDLE_INVALID;
  MojoHandleSignals sig = ~MOJO_HANDLE_SIGNAL_NONE;
  EXPECT_EQ(
      MOJO_RESULT_INVALID_ARGUMENT,
      MojoWaitMany(&h, &sig, 1u, MOJO_DEADLINE_INDEFINITE, nullptr, nullptr));
}

// TODO(vtl): Write tests that actually test waiting.

}  // namespace
