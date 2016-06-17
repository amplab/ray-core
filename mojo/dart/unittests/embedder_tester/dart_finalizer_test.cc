// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/dart/unittests/embedder_tester/dart_test.h"
#include "mojo/public/cpp/environment/environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace dart {
namespace {

class DartFinalizerTest : public DartTest {
 public:
  DartFinalizerTest() {}

  static void SetUpTestCase() {
    const int kNumArgs = 2;
    const char* args[kNumArgs];
    args[0] = "--new-gen-semi-max-size=1";
    args[1] = "--old_gen_growth_rate=1";
    DartController::Initialize(nullptr, true, false, false, args, kNumArgs);
  }

  void RunTest(const std::string& test) {
    base::FilePath path;
    PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.AppendASCII("mojo")
               .AppendASCII("dart")
               .AppendASCII("unittests")
               .AppendASCII("embedder_tests")
               .AppendASCII(test);
    std::vector<std::string> script_arguments;

    RunDartTest(path, script_arguments, false, false, 0);
  }
};

TEST_F(DartFinalizerTest, handle_finalizer_test) {
  RunTest("handle_finalizer_test.dart");
}

TEST_F(DartFinalizerTest, shared_buffer_finalizer_test) {
  RunTest("shared_buffer_finalizer_test.dart");
}

}  // namespace
}  // namespace dart
}  // namespace mojo
