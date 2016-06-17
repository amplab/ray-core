// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This example demonstrates the simple use of SynchronousInterfacePtr<>, which
// is the blocking, synchronous version of mojom interface calls (typically used
// via InterfacePtr<>).

#include "examples/echo/echo.mojom-sync.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "mojo/public/cpp/environment/logging.h"
#include "mojo/public/cpp/utility/run_loop.h"

namespace mojo {
namespace examples {

class EchoClientApp : public ApplicationImplBase {
 public:
  void OnInitialize() override {
    SynchronousInterfacePtr<Echo> echo;
    ConnectToService(shell(), "mojo:echo_server", GetSynchronousProxy(&echo));

    mojo::String out = "yo!";
    MOJO_CHECK(echo->EchoString("hello", &out));
    MOJO_LOG(INFO) << "Got response: " << out;
    RunLoop::current()->Quit();
  }
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::examples::EchoClientApp echo_client_app;
  return mojo::RunApplication(application_request, &echo_client_app);
}
