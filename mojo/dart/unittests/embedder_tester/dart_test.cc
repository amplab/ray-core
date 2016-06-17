// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/dart/unittests/embedder_tester/dart_test.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/public/cpp/environment/environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace dart {

void DartTest::RunDartTest(const base::FilePath& path,
                           const std::vector<std::string>& script_arguments,
                           bool compile_all,
                           bool expect_unhandled_exception,
                           int expected_unclosed_handles) {
  // Grab the C string pointer.
  std::vector<const char*> script_arguments_c_str;
  for (size_t i = 0; i < script_arguments.size(); i++) {
    script_arguments_c_str.push_back(script_arguments[i].c_str());
  }
  DCHECK(script_arguments.size() == script_arguments_c_str.size());

  // Setup the package root.
  base::FilePath package_root;
  PathService::Get(base::DIR_EXE, &package_root);
  package_root = package_root.AppendASCII("gen")
                             .AppendASCII("dart-pkg")
                             .AppendASCII("packages");

  char* error = nullptr;
  bool unhandled_exception = false;
  int64_t closed_handles = 0;
  DartControllerConfig config;
  // Run with strict compilation even in Release mode so that ASAN testing gets
  // coverage of Dart asserts, type-checking, etc.
  config.strict_compilation = true;
  config.compile_all = compile_all;
  config.script_uri = path.AsUTF8Unsafe();
  config.base_uri = path.AsUTF8Unsafe();
  config.package_root = package_root.AsUTF8Unsafe();
  config.callbacks.exception =
      base::Bind(&ExceptionCallback, &unhandled_exception, &closed_handles);
  config.entropy = GenerateEntropy;
  config.error = &error;

  if (script_arguments_c_str.size() > 0) {
    config.SetScriptFlags(script_arguments_c_str.data(),
                          script_arguments_c_str.size());
  }

  Dart_Isolate isolate = DartController::StartupIsolate(config);
  EXPECT_TRUE(isolate != nullptr) << error;
  DartController::RunToCompletion(isolate);
  DartController::ShutdownIsolate(isolate);
  EXPECT_EQ(expect_unhandled_exception, unhandled_exception);
  EXPECT_EQ(expected_unclosed_handles, closed_handles);
}

}  // namespace dart
}  // namespace mojo
