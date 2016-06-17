// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/http_server/interfaces/http_server_factory.mojom.h"
#include "services/http_server/http_server_factory_impl.h"
#include "services/http_server/http_server_impl.h"

namespace http_server {

class HttpServerApp : public mojo::ApplicationImplBase {
 public:
  HttpServerApp() {}
  ~HttpServerApp() override {}

 private:
  // ApplicationImplBase:
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<HttpServerFactory>([this](
        const mojo::ConnectionContext& connection_context,
        mojo::InterfaceRequest<HttpServerFactory> http_server_factory_request) {
      if (!http_server_factory_)
        http_server_factory_.reset(new HttpServerFactoryImpl(shell()));

      http_server_factory_->AddBinding(http_server_factory_request.Pass());
    });
    return true;
  }

  std::unique_ptr<HttpServerFactoryImpl> http_server_factory_;
};

}  // namespace http_server

MojoResult MojoMain(MojoHandle application_request) {
  http_server::HttpServerApp http_server_app;
  return mojo::RunApplication(application_request, &http_server_app);
}
