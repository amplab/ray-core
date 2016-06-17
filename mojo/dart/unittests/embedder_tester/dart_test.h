// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_DART_UNITTESTS_EMBEDDER_TESTER_DART_TEST_H_
#define MOJO_DART_UNITTESTS_EMBEDDER_TESTER_DART_TEST_H_

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/public/cpp/environment/environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace dart {

class DartTest : public testing::Test {
 public:
  DartTest() {}

  static void SetUpTestCase() {
    DartController::Initialize(nullptr, true, false, false, nullptr, 0);
  }

  static void TearDownTestCase() {
    DartController::Shutdown();
  }

  void RunDartTest(const base::FilePath& path,
                   const std::vector<std::string>& script_arguments,
                   bool compile_all,
                   bool expect_unhandled_exception,
                   int expected_unclosed_handles);

 protected:
  static bool GenerateEntropy(uint8_t* buffer, intptr_t length) {
    base::RandBytes(static_cast<void*>(buffer), length);
    return true;
  }

  static void ExceptionCallback(bool* exception,
                                int64_t* closed_handles,
                                Dart_Handle error,
                                int64_t count) {
    EXPECT_TRUE(Dart_IsError(error));
    *exception = true;
    *closed_handles = count;
  }
};

}  // namespace dart
}  // namespace mojo

#endif  // MOJO_DART_UNITTESTS_EMBEDDER_TESTER_DART_TEST_H_
