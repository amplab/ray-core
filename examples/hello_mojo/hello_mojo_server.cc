// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "examples/hello_mojo/hello_mojo.mojom.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/macros.h"

using examples::HelloMojo;

namespace {

class HelloMojoImpl : public HelloMojo {
 public:
  explicit HelloMojoImpl(mojo::InterfaceRequest<HelloMojo> hello_mojo_request)
      : strong_binding_(this, std::move(hello_mojo_request)) {}
  ~HelloMojoImpl() override {}

  // |examples::HelloMojo| implementation:
  void Say(const mojo::String& request,
           const mojo::Callback<void(mojo::String)>& callback) override {
    callback.Run((request.get() == "hello") ? "mojo" : "WAT");
  }

 private:
  mojo::StrongBinding<HelloMojo> strong_binding_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(HelloMojoImpl);
};

class HelloMojoServerApp : public mojo::ApplicationImplBase {
 public:
  HelloMojoServerApp() {}
  ~HelloMojoServerApp() override {}

  // |mojo::ApplicationImplBase| override:
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<HelloMojo>(
        [](const mojo::ConnectionContext& connection_context,
           mojo::InterfaceRequest<HelloMojo> hello_mojo_request) {
          new HelloMojoImpl(std::move(hello_mojo_request));  // Owns itself.
        });
    return true;
  }

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(HelloMojoServerApp);
};

}  // namespace

MojoResult MojoMain(MojoHandle application_request) {
  HelloMojoServerApp hello_mojo_server_app;
  return mojo::RunApplication(application_request, &hello_mojo_server_app);
}
