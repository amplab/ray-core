// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "gin/modules/module_registry.h"
#include "gin/modules/timer.h"
#include "gin/test/file_runner.h"
#include "gin/test/gtest.h"
#include "services/js/modules/clock/monotonic_clock.h"
#include "services/js/system/threading.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace js {
namespace {

class TestRunnerDelegate : public gin::FileRunnerDelegate {
 public:
  TestRunnerDelegate() {
    AddBuiltinModule(gin::TimerModule::kName, gin::TimerModule::GetModule);
    AddBuiltinModule(MonotonicClock::kModuleName, MonotonicClock::GetModule);
    AddBuiltinModule(
        mojo::js::Threading::kModuleName, mojo::js::Threading::GetModule);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestRunnerDelegate);
};

void RunTest(std::string test, bool run_until_idle) {
  base::FilePath path;
  PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.AppendASCII("services")
             .AppendASCII("js")
             .AppendASCII("modules")
             .AppendASCII("clock")
             .AppendASCII(test);
  TestRunnerDelegate delegate;
  gin::RunTestFromFile(path, &delegate, run_until_idle);
}

// TODO(abarth): Should we autogenerate these stubs from GYP?

TEST(JSTest, monotonic_clock) {
  RunTest("monotonic_clock_unittests.js", false);
}

}  // namespace
}  // namespace js
