// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/dart/unittests/embedder_tester/dart_test.h"
#include "mojo/public/cpp/environment/environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace dart {
namespace {

class DartUnitTest : public DartTest {
 public:
  DartUnitTest() {}

  void RunTest(const std::string& test,
               bool compile_all,
               bool expect_unhandled_exception,
               int expected_unclosed_handles) {
    base::FilePath path;
    PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.AppendASCII("mojo")
               .AppendASCII("dart")
               .AppendASCII("unittests")
               .AppendASCII("embedder_tests")
               .AppendASCII(test);
    std::vector<std::string> script_arguments;

    RunDartTest(path, script_arguments, compile_all,
                expect_unhandled_exception, expected_unclosed_handles);
  }
};

// TODO(zra): instead of listing all these tests, search //mojo/dart/test for
// _test.dart files.

TEST_F(DartUnitTest, async_await_test) {
  RunTest("async_await_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, async_test) {
  RunTest("async_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, bindings_generation_test) {
  RunTest("bindings_generation_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, codec_test) {
  RunTest("codec_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, compile_all_interfaces_test) {
  RunTest("compile_all_interfaces_test.dart", true, false, 0);
}

TEST_F(DartUnitTest, control_messages_test) {
  RunTest("control_messages_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, core_test) {
  RunTest("core_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, core_types_test) {
  RunTest("core_types_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, exception_test) {
  RunTest("exception_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, handle_finalizer_test) {
  RunTest("handle_finalizer_test.dart", false, true, 1);
}

TEST_F(DartUnitTest, hello_mojo) {
  RunTest("hello_mojo.dart", false, false, 0);
}

TEST_F(DartUnitTest, isolate_test) {
  RunTest("isolate_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, import_mojo) {
  RunTest("import_mojo.dart", false, false, 0);
}

TEST_F(DartUnitTest, ping_pong_test) {
  RunTest("ping_pong_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, simple_handle_watcher_test) {
  RunTest("simple_handle_watcher_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, timer_test) {
  RunTest("timer_test.dart", false, false, 0);
}

TEST_F(DartUnitTest, unhandled_exception_test) {
  RunTest("unhandled_exception_test.dart", false, true, 2);
}

TEST_F(DartUnitTest, uri_base_test) {
  RunTest("uri_base_test.dart", false, false, 0);
}

}  // namespace
}  // namespace dart
}  // namespace mojo
