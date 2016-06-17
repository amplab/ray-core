// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/js/test/js_application_test_base.h"
#include "services/js/test/pingpong_service.mojom.h"

namespace js {
namespace {

class PingPongClientImpl : public PingPongClient {
 public:
  PingPongClientImpl() : last_pong_value_(0), binding_(this) {};
  ~PingPongClientImpl() override {};

  void Bind(mojo::InterfaceRequest<PingPongClient> request) {
    binding_.Bind(request.Pass());
  }

  int16_t WaitForPongValue() {
    if (!binding_.WaitForIncomingMethodCall())
      return 0;
    return last_pong_value_;
  }

 private:
  // PingPongClient overrides.
  void Pong(uint16_t pong_value) override {
    last_pong_value_ = pong_value;
  }

  int16_t last_pong_value_;
  mojo::Binding<PingPongClient> binding_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(PingPongClientImpl);
};

class JSPingPongTest : public test::JSApplicationTestBase {
 public:
  JSPingPongTest() : JSApplicationTestBase() {}
  ~JSPingPongTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();
    const std::string& url = JSAppURL("pingpong.js");
    mojo::ConnectToService(shell(), url, GetProxy(&pingpong_service_));
    PingPongClientPtr client_ptr;
    pingpong_client_.Bind(GetProxy(&client_ptr));
    pingpong_service_->SetClient(client_ptr.Pass());
  }

  PingPongServicePtr pingpong_service_;
  PingPongClientImpl pingpong_client_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(JSPingPongTest);
};

struct PingTargetCallback {
  explicit PingTargetCallback(bool *b) : value(b) {}
  void Run(bool callback_value) const {
    *value = callback_value;
  }
  bool *value;
};

// Verify that "pingpong.js" implements the PingPongService interface
// and sends responses to our client.
TEST_F(JSPingPongTest, PingServiceToPongClient) {
  pingpong_service_->Ping(1);
  EXPECT_EQ(2, pingpong_client_.WaitForPongValue());
  pingpong_service_->Ping(100);
  EXPECT_EQ(101, pingpong_client_.WaitForPongValue());
  pingpong_service_->Quit();
}

// Verify that "pingpong.js" can connect to "pingpong-target.js", act as
// its client, and return a Promise that only resolves after the target.ping()
// => client.pong() methods have executed 9 times.
TEST_F(JSPingPongTest, PingTargetURL) {
  bool returned_value = false;
  PingTargetCallback callback(&returned_value);
  pingpong_service_->PingTargetURL(JSAppURL("pingpong_target.js"), 9, callback);
  EXPECT_TRUE(pingpong_service_.WaitForIncomingResponse());
  EXPECT_TRUE(returned_value);
  pingpong_service_->Quit();
}

// Same as the previous test except that instead of providing the
// pingpong-target.js URL, we provide a connection to its PingPongService.
TEST_F(JSPingPongTest, PingTargetService) {
  PingPongServicePtr target;
  mojo::ConnectToService(shell(), JSAppURL("pingpong_target.js"),
                         GetProxy(&target));
  bool returned_value = false;
  PingTargetCallback callback(&returned_value);
  pingpong_service_->PingTargetService(target.Pass(), 9, callback);
  EXPECT_TRUE(pingpong_service_.WaitForIncomingResponse());
  EXPECT_TRUE(returned_value);
  pingpong_service_->Quit();
}

// Verify that JS can implement an interface& "request" parameter.
TEST_F(JSPingPongTest, GetTargetService) {
  PingPongServicePtr target;
  PingPongClientImpl client;
  pingpong_service_->GetPingPongService(GetProxy(&target));
  PingPongClientPtr client_ptr;
  client.Bind(GetProxy(&client_ptr));
  target->SetClient(client_ptr.Pass());
  target->Ping(1);
  EXPECT_EQ(2, client.WaitForPongValue());
  target->Ping(100);
  EXPECT_EQ(101, client.WaitForPongValue());
  target->Quit();
  pingpong_service_->Quit();
}


}  // namespace
}  // namespace js
