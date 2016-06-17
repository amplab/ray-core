// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C time API (the functions declared in
// mojo/public/c/system/time.h).

#include "mojo/public/c/system/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(TimeTest, GetTimeTicksNow) {
  const MojoTimeTicks start = MojoGetTimeTicksNow();
  EXPECT_NE(static_cast<MojoTimeTicks>(0), start)
      << "MojoGetTimeTicksNow should return nonzero value";
}

}  // namespace
