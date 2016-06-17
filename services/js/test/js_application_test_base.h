// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_JS_TEST_JS_APPLICATION_TEST_BASE_H_
#define SERVICES_JS_TEST_JS_APPLICATION_TEST_BASE_H_

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "mojo/public/cpp/application/application_test_base.h"

namespace js {
namespace test {

class JSApplicationTestBase : public mojo::test::ApplicationTestBase {
 public:
  JSApplicationTestBase();
  ~JSApplicationTestBase() override;

 protected:
  const base::FilePath JSAppPath(const std::string& filename);
  const std::string JSAppURL(const std::string& filename);

  MOJO_DISALLOW_COPY_AND_ASSIGN(JSApplicationTestBase);
};

}  // namespace test
}  // namespace js

#endif  // SERVICES_JS_TEST_JS_APPLICATION_TEST_BASE_H_
