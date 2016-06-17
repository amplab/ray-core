// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/services/http_server/cpp/http_server_util.h"
#include "mojo/services/http_server/interfaces/http_server.mojom.h"
#include "mojo/services/http_server/interfaces/http_server_factory.mojom.h"
#include "mojo/services/network/interfaces/net_address.mojom.h"

namespace mojo {
namespace examples {

// This is an example of a self-contained HTTP handler. It uses the HTTP Server
// service to handle the HTTP protocol details, and just contains the logic for
// handling its registered urls.
class HttpHandler : public ApplicationImplBase,
                    public http_server::HttpHandler {
 public:
  HttpHandler() : binding_(this) {}
  ~HttpHandler() override {}

 private:
  // ApplicationImplBase override:
  void OnInitialize() override {
    http_server::HttpHandlerPtr http_handler_ptr;
    binding_.Bind(GetProxy(&http_handler_ptr));

    http_server::HttpServerFactoryPtr http_server_factory;
    ConnectToService(shell(), "mojo:http_server",
                     GetProxy(&http_server_factory));

    mojo::NetAddressPtr local_address(mojo::NetAddress::New());
    local_address->family = mojo::NetAddressFamily::IPV4;
    local_address->ipv4 = mojo::NetAddressIPv4::New();
    local_address->ipv4->addr.resize(4);
    local_address->ipv4->addr[0] = 0;
    local_address->ipv4->addr[1] = 0;
    local_address->ipv4->addr[2] = 0;
    local_address->ipv4->addr[3] = 0;
    local_address->ipv4->port = 8080;
    http_server_factory->CreateHttpServer(GetProxy(&http_server_).Pass(),
                                          local_address.Pass());

    http_server_->SetHandler(
        "/test",
        http_handler_ptr.Pass(),
        base::Bind(&HttpHandler::AddHandlerCallback, base::Unretained(this)));
  }

  // http_server::HttpHandler:
  void HandleRequest(
      http_server::HttpRequestPtr request,
      const Callback<void(http_server::HttpResponsePtr)>& callback) override {
    callback.Run(http_server::CreateHttpResponse(200, "Hello World\n"));
  }

  void AddHandlerCallback(bool result) {
    CHECK(result);
  }

  Binding<http_server::HttpHandler> binding_;
  http_server::HttpServerPtr http_server_;

  DISALLOW_COPY_AND_ASSIGN(HttpHandler);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  mojo::examples::HttpHandler http_handler;
  return mojo::RunApplication(application_request, &http_handler);
}
