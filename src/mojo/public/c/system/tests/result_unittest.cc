// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C result API (the macros declared in
// mojo/public/c/system/result.h).

#include "mojo/public/c/system/result.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(ResultTest, Macros) {
  EXPECT_EQ(0x12345678u, MOJO_RESULT_MAKE(static_cast<MojoResult>(0x8u),
                                          static_cast<MojoResult>(0x1234u),
                                          static_cast<MojoResult>(0x567u)));
  EXPECT_EQ(0x8u, MOJO_RESULT_GET_CODE(static_cast<MojoResult>(0x12345678u)));
  EXPECT_EQ(0x1234u,
            MOJO_RESULT_GET_SPACE(static_cast<MojoResult>(0x12345678u)));
  EXPECT_EQ(0x567u,
            MOJO_RESULT_GET_SUBCODE(static_cast<MojoResult>(0x12345678u)));

  EXPECT_EQ(0xfedcfedfu, MOJO_RESULT_MAKE(static_cast<MojoResult>(0xfu),
                                          static_cast<MojoResult>(0xfedcu),
                                          static_cast<MojoResult>(0xfedu)));
  EXPECT_EQ(0xfu, MOJO_RESULT_GET_CODE(static_cast<MojoResult>(0xfedcfedfu)));
  EXPECT_EQ(0xfedcu,
            MOJO_RESULT_GET_SPACE(static_cast<MojoResult>(0xfedcfedfu)));
  EXPECT_EQ(0xfedu,
            MOJO_RESULT_GET_SUBCODE(static_cast<MojoResult>(0xfedcfedfu)));
}

TEST(ResultTest, CompleteResultMacros) {
  EXPECT_EQ(0x0u, MOJO_RESULT_OK);
  EXPECT_EQ(0x1u, MOJO_RESULT_CANCELLED);
  EXPECT_EQ(0x2u, MOJO_RESULT_UNKNOWN);
  EXPECT_EQ(0x3u, MOJO_RESULT_INVALID_ARGUMENT);
  EXPECT_EQ(0x4u, MOJO_RESULT_DEADLINE_EXCEEDED);
  EXPECT_EQ(0x5u, MOJO_RESULT_NOT_FOUND);
  EXPECT_EQ(0x6u, MOJO_RESULT_ALREADY_EXISTS);
  EXPECT_EQ(0x7u, MOJO_RESULT_PERMISSION_DENIED);
  EXPECT_EQ(0x8u, MOJO_RESULT_RESOURCE_EXHAUSTED);
  EXPECT_EQ(0x9u, MOJO_RESULT_FAILED_PRECONDITION);
  EXPECT_EQ(0xau, MOJO_RESULT_ABORTED);
  EXPECT_EQ(0xbu, MOJO_RESULT_OUT_OF_RANGE);
  EXPECT_EQ(0xcu, MOJO_RESULT_UNIMPLEMENTED);
  EXPECT_EQ(0xdu, MOJO_RESULT_INTERNAL);
  EXPECT_EQ(0xeu, MOJO_RESULT_UNAVAILABLE);
  EXPECT_EQ(0xfu, MOJO_RESULT_DATA_LOSS);
  EXPECT_EQ(0x0019u, MOJO_RESULT_BUSY);
  EXPECT_EQ(0x001eu, MOJO_RESULT_SHOULD_WAIT);
}

}  // namespace
