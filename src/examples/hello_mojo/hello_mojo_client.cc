// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include <string>

#include "examples/hello_mojo/hello_mojo.mojom.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/utility/run_loop.h"

using examples::HelloMojoPtr;

namespace {

class HelloMojoClientApp : public mojo::ApplicationImplBase {
 public:
  HelloMojoClientApp() {}
  ~HelloMojoClientApp() override {}

  void OnInitialize() override {
    mojo::ConnectToService(shell(), "mojo:hello_mojo_server",
                           GetProxy(&hello_mojo_));

    DoIt("hello");
    DoIt("goodbye");
  }

 private:
  void DoIt(const std::string& request) {
    hello_mojo_->Say(request, [request](const mojo::String& response) {
      printf("%s --> %s\n", request.c_str(), response.get().c_str());
    });
  }

  HelloMojoPtr hello_mojo_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(HelloMojoClientApp);
};

}  // namespace

MojoResult MojoMain(MojoHandle application_request) {
  HelloMojoClientApp hello_mojo_client_app;
  return mojo::RunApplication(application_request, &hello_mojo_client_app);
}
