// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_TEST_BASE_H_
#define MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_TEST_BASE_H_

#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/array.h"
#include "mojo/public/cpp/bindings/string.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {

// Run all application tests. This must be called after the environment is
// initialized, to support construction of a default run loop.
MojoResult RunAllTests(MojoHandle application_request_handle);

// A GTEST base class for application testing executed in mojo_shell.
class ApplicationTestBase : public testing::Test {
 public:
  ApplicationTestBase();
  ~ApplicationTestBase() override;

 protected:
  Shell* shell() const { return shell_.get(); }
  const std::vector<std::string>& args() const { return args_; }

  // testing::Test:
  void SetUp() override;
  void TearDown() override;

  // True by default, which indicates a MessageLoop will automatically be
  // created for the application.  Tests may override this function to prevent
  // a default loop from being created.
  virtual bool ShouldCreateDefaultRunLoop();

 private:
  ShellPtr shell_;
  std::vector<std::string> args_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ApplicationTestBase);
};

}  // namespace test
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_TEST_BASE_H_
