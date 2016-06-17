// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "examples/apptest/example_service.mojom.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/callback.h"
#include "mojo/public/cpp/environment/logging.h"
#include "mojo/public/cpp/system/macros.h"

namespace mojo {
namespace {

// Exemplifies ApplicationTestBase's application testing pattern.
class ExampleApplicationTest : public test::ApplicationTestBase {
 public:
  ExampleApplicationTest() : ApplicationTestBase() {}
  ~ExampleApplicationTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();

    ConnectToService(shell(), "mojo:example_service",
                     GetProxy(&example_service_));
  }

  ExampleServicePtr example_service_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(ExampleApplicationTest);
};

TEST_F(ExampleApplicationTest, PingServiceToPong) {
  uint16_t pong_value = 0u;
  example_service_->Ping(1u,
                         [&pong_value](uint16_t value) { pong_value = value; });
  EXPECT_TRUE(example_service_.WaitForIncomingResponse());
  // Test making a call and receiving the reply.
  EXPECT_EQ(1u, pong_value);
}

TEST_F(ExampleApplicationTest, CheckCommandLineArg) {
  // Ensure the test runner passes along this example command line argument.
  ASSERT_TRUE(std::find(args().begin(), args().end(),
                        "--example_apptest_arg") != args().end());
}

}  // namespace
}  // namespace mojo
