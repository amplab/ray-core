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

#include "base/message_loop/message_loop.h"

#include <iostream>

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

    Terminate(MOJO_RESULT_OK);
  }
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::examples::EchoClientApp echo_client_app;
  mojo::SynchronousInterfacePtr<mojo::examples::Echo> echo;
  std::unique_ptr<base::MessageLoop> loop(new base::MessageLoop(common::MessagePumpMojo::Create()));
  echo_client_app.Bind(mojo::InterfaceRequest<mojo::Application>(
      mojo::MakeScopedHandle(mojo::MessagePipeHandle(application_request))));
  mojo::ConnectToService(echo_client_app.shell(), "mojo:echo_server", mojo::GetSynchronousProxy(&echo));
  mojo::String out = "yo!";
  MOJO_CHECK(echo->EchoString("hello", &out));
  std::cout << "result: " << out << std::endl;
  return MOJO_RESULT_OK;
  // return mojo::RunApplication(application_request, &echo_client_app);
}
