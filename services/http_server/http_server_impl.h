// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "mojo/common/binding_set.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/http_server/interfaces/http_request.mojom.h"
#include "mojo/services/http_server/interfaces/http_response.mojom.h"
#include "mojo/services/http_server/interfaces/http_server.mojom.h"
#include "mojo/services/network/interfaces/net_address.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "third_party/re2/re2/re2.h"

namespace mojo {
class Shell;
}  // namespace mojo

namespace http_server {

class Connection;
class HttpServerFactoryImpl;

class HttpServerImpl : public HttpServer {
 public:
  // TODO(vtl): Possibly this should take an
  // |InterfaceHandle<ApplicationConnector>| instead of a |Shell*|.
  HttpServerImpl(mojo::Shell* shell,
                 HttpServerFactoryImpl* factory,
                 mojo::NetAddressPtr requested_local_address);
  ~HttpServerImpl() override;

  void AddBinding(mojo::InterfaceRequest<HttpServer> request);

  // HttpServer:
  void SetHandler(const mojo::String& path,
                  mojo::InterfaceHandle<HttpHandler> http_handler,
                  const mojo::Callback<void(bool)>& callback) override;

  void GetPort(const GetPortCallback& callback) override;

 private:
  struct Handler {
    Handler(const std::string& pattern, HttpHandlerPtr http_handler);
    ~Handler();
    scoped_ptr<re2::RE2> pattern;
    HttpHandlerPtr http_handler;

   private:
    DISALLOW_COPY_AND_ASSIGN(Handler);
  };

  void OnHandlerConnectionError(Handler* handler);

  void OnSocketBound(mojo::NetworkErrorPtr err,
                     mojo::NetAddressPtr bound_address);

  void OnSocketListening(mojo::NetworkErrorPtr err);

  void OnConnectionAccepted(mojo::NetworkErrorPtr err,
                            mojo::NetAddressPtr remote_address);

  void WaitForNextConnection();

  void Start();

  void HandleRequest(Connection* connection, HttpRequestPtr request);

  void OnResponse(Connection* connection, HttpResponsePtr response);

  HttpServerFactoryImpl* factory_;

  mojo::NetAddressPtr requested_local_address_;
  uint16_t assigned_port_;
  std::vector<GetPortCallback> pending_get_port_callbacks_;

  mojo::BindingSet<HttpServer> bindings_;

  mojo::NetworkServicePtr network_service_;
  mojo::TCPBoundSocketPtr bound_socket_;
  mojo::TCPServerSocketPtr server_socket_;

  mojo::ScopedDataPipeProducerHandle pending_send_handle_;
  mojo::ScopedDataPipeConsumerHandle pending_receive_handle_;
  mojo::TCPConnectedSocketPtr pending_connected_socket_;

  // TODO(vtl): Maybe this should be an std::set or unordered_set, which would
  // simplify OnHandlerConnectionError().
  ScopedVector<Handler> handlers_;

  base::WeakPtrFactory<HttpServerImpl> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(HttpServerImpl);
};

}  // namespace http_server
