// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/buffer.h"

#include <stdint.h>

#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(MojomBufferTest, RoundTo8) {
  char buffer[100];
  struct MojomBuffer mbuf = {
      buffer, sizeof(buffer),
      0,  // num_bytes_used
  };

  // This should actually allocate 8 bytes:
  EXPECT_EQ(buffer, MojomBuffer_Allocate(&mbuf, 6));
  EXPECT_EQ(8ul, mbuf.num_bytes_used);
  // The start of the next buffer is buffer + 8 bytes:
  EXPECT_EQ(buffer + 8, MojomBuffer_Allocate(&mbuf, 6));
  EXPECT_EQ(16ul, mbuf.num_bytes_used);
  // 8-bye allocation results in 8-byte allocation.
  EXPECT_EQ(buffer + 16, MojomBuffer_Allocate(&mbuf, 8));
  EXPECT_EQ(24ul, mbuf.num_bytes_used);
  // Allocate 0 bytes.
  EXPECT_EQ(buffer + 24ul, MojomBuffer_Allocate(&mbuf, 0));
  EXPECT_EQ(24ul, mbuf.num_bytes_used);
}

TEST(MojomBufferTest, Failure) {
  char buffer[100];
  struct MojomBuffer mbuf = {
      buffer, sizeof(buffer),
      0,  // num_bytes_used
  };

  // Allocate too much space.
  EXPECT_EQ(NULL, MojomBuffer_Allocate(&mbuf, sizeof(buffer) + 10));

  // Setup the buffer data so that it will overflow.
  mbuf.num_bytes_used = UINT32_MAX - 7;  // This divides by 8.
  mbuf.buf_size = UINT32_MAX;

  // Rounds to 8, and allocates more than it has:
  EXPECT_EQ(NULL, MojomBuffer_Allocate(&mbuf, 1));
}

}  // namespace
