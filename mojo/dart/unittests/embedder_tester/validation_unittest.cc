// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <set>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "base/strings/string_util.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/dart/unittests/embedder_tester/dart_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace dart {
namespace {

class DartValidationTest : public DartTest {
 public:

  DartValidationTest() {}

  std::string GetPath() {
    base::FilePath path;
    PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.AppendASCII("mojo")
               .AppendASCII("public")
               .AppendASCII("interfaces")
               .AppendASCII("bindings")
               .AppendASCII("tests")
               .AppendASCII("data")
               .AppendASCII("validation");

    return path.AsUTF8Unsafe();
  }

  void RunTest(const std::string& test) {
    base::FilePath path;
    PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.AppendASCII("mojo")
               .AppendASCII("dart")
               .AppendASCII("unittests")
               .AppendASCII("embedder_tests")
               .AppendASCII("validation_test.dart");
    std::vector<std::string> script_arguments =
        CollectTests(base::FilePath(test));

    RunDartTest(path, script_arguments, false, false, 0);
  }

 private:
  // Enumerates files inside |path| and collects all data needed to run
  // conformance tests.
  // For each test there are three entries in the returned vector.
  // [0] -> test name.
  // [1] -> contents of test's .data file.
  // [2] -> contents of test's .expected file.
  static std::vector<std::string> CollectTests(base::FilePath path) {
    base::FileEnumerator enumerator(path, false, base::FileEnumerator::FILES);
    std::set<std::string> tests;
    while (true) {
      base::FilePath file_path = enumerator.Next();
      if (file_path.empty()) {
        break;
      }
      file_path = file_path.RemoveExtension();
      tests.insert(file_path.value());
    }
    std::vector<std::string> result;
    for (auto it = tests.begin(); it != tests.end(); it++) {
      const std::string& test_name = *it;
      std::string source;
      bool r;
      std::string filename = base::FilePath(test_name).BaseName().value();
      if (!base::StartsWith(filename, "conformance_",
                            base::CompareCase::SENSITIVE)) {
        // Only include conformance tests.
        continue;
      }
      base::FilePath data_path(test_name);
      data_path = data_path.AddExtension(".data");
      base::FilePath expected_path(test_name);
      expected_path = expected_path.AddExtension(".expected");
      // Test name.
      result.push_back(test_name.c_str());
      // Test's .data.
      r = ReadFileToString(data_path, &source);
      DCHECK(r);
      result.push_back(source);
      source.clear();
      // Test's .expected.
      r = ReadFileToString(expected_path, &source);
      DCHECK(r);
      result.push_back(source);
      source.clear();
    }
    return result;
  }
};

TEST_F(DartValidationTest, validation) {
  RunTest(GetPath());
}

}  // namespace
}  // namespace dart
}  // namespace mojo
