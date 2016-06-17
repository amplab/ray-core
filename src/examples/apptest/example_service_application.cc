// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/apptest/example_service_application.h"

#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"

namespace mojo {

ExampleServiceApplication::ExampleServiceApplication() {}

ExampleServiceApplication::~ExampleServiceApplication() {}

bool ExampleServiceApplication::OnAcceptConnection(
    ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<ExampleService>(
      [](const ConnectionContext& connection_context,
         InterfaceRequest<ExampleService> example_service_request) {
        // Not leaked: ExampleServiceImpl is strongly bound to the pipe.
        new ExampleServiceImpl(example_service_request.Pass());
      });
  return true;
}

}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ExampleServiceApplication example_service_application;
  return mojo::RunApplication(application_request,
                              &example_service_application);
}
