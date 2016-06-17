// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/js/test/js_application_test_base.h"

namespace js {
namespace test {

JSApplicationTestBase::JSApplicationTestBase() {}
JSApplicationTestBase::~JSApplicationTestBase() {}

const base::FilePath JSApplicationTestBase::JSAppPath(
    const std::string& filename) {
 base::FilePath path;
  PathService::Get(base::DIR_SOURCE_ROOT, &path);
  return path.AppendASCII("services")
      .AppendASCII("js")
      .AppendASCII("test")
      .AppendASCII(filename);
}

const std::string JSApplicationTestBase::JSAppURL(const std::string& filename) {
  return "file://" + JSAppPath(filename).AsUTF8Unsafe();
}

}  // namespace test
}  // namespace mojo

