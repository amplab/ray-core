// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file just "forwards" to "pure" tests -- those that are written using
// only headers from mojo/public/c/system (and the C standard library), and not
// using gtest.

#include "testing/gtest/include/gtest/gtest.h"

// Defined in compile_unittest_pure_c.c.
extern "C" const char* MinimalCTest(void);

// Defined in compile_unittest_pure_cpp.cc.
const char* MinimalCppTest();

namespace {

// This checks that things actually work in C (not C++).
TEST(CompileTest, MinimalCTest) {
  const char* failure = MinimalCTest();
  EXPECT_FALSE(failure) << failure;
}

// This checks that things compile in C++ as well, without gtest.
TEST(CompileTest, MinimalCppTest) {
  const char* failure = MinimalCppTest();
  EXPECT_FALSE(failure) << failure;
}

}  // namespace
