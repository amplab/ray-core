// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "examples/echo/echo.mojom.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/utility/run_loop.h"

namespace mojo {
namespace examples {

class ResponsePrinter {
 public:
  void Run(const uint64& value) const {
    auto stop = std::chrono::high_resolution_clock::now();
    auto nanos = duration_cast<nanoseconds>(start.time_since_epoch()).count();
    std::cout << nanos - value << " ns" << std::endl;
  }
};

class EchoClientApp : public ApplicationImplBase {
 public:
  void OnInitialize() override {
    ConnectToService(shell(), "mojo:echo_server", GetProxy(&echo_));
    auto start = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(start.time_since_epoch()).count();
    echo_->EchoString(nanos, ResponsePrinter());
  }

 private:
  EchoPtr echo_;
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::examples::EchoClientApp echo_client_app;
  return mojo::RunApplication(application_request, &echo_client_app);
}
