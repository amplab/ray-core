// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/gpu/MGL/mgl.h"

#include <string.h>

#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

TEST(MGLTest, GetProcAddress) {
  EXPECT_EQ(nullptr, MGLGetProcAddress(""));
  EXPECT_NE(nullptr, MGLGetProcAddress("glActiveTexture"));
}

}  // namespace
}  // namespace mojo
