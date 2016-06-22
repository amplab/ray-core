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
  void Run(const String& value) const {
    LOG(INFO) << "***** Response: " << value.get().c_str();
    RunLoop::current()->Quit();  // All done!
  }
};

class EchoClientApp : public ApplicationImplBase {
 public:
  void OnInitialize() override {
    ConnectToService(shell(), "mojo:echo_server", GetProxy(&echo_));

    echo_->EchoString("hello world", ResponsePrinter());
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
