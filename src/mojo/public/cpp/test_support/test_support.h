// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Support functions for running "internal" tests. These functions are not for
// public consumption.

#ifndef MOJO_PUBLIC_CPP_TEST_SUPPORT_TEST_SUPPORT_H_
#define MOJO_PUBLIC_CPP_TEST_SUPPORT_TEST_SUPPORT_H_

#include <string>
#include <vector>

namespace mojo {
namespace test {

// Logs the result of a perf test. (|sub_test_name| is optional and may be
// null.) You may only call this while running inside a perf test suite.
void LogPerfResult(const char* test_name,
                   const char* sub_test_name,
                   double value,
                   const char* units);

// Opens a text file given by a path relative to the source root for reading.
FILE* OpenSourceRootRelativeFile(const std::string& relative_path);

// Returns the list of regular files in a directory given by a path relative to
// the source root. (The result only includes the filenames, not the path.)
std::vector<std::string> EnumerateSourceRootRelativeDirectory(
    const std::string& relative_path);

}  // namespace test
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_TEST_SUPPORT_TEST_SUPPORT_H_
