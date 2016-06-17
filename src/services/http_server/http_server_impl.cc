// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/http_server/http_server_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/services/http_server/cpp/http_server_util.h"
#include "services/http_server/connection.h"
#include "services/http_server/http_server_factory_impl.h"

namespace http_server {

HttpServerImpl::HttpServerImpl(mojo::Shell* shell,
                               HttpServerFactoryImpl* factory,
                               mojo::NetAddressPtr requested_local_address)
    : factory_(factory),
      requested_local_address_(requested_local_address.Pass()),
      assigned_port_(0),
      weak_ptr_factory_(this) {
  mojo::ConnectToService(shell, "mojo:network_service",
                         GetProxy(&network_service_));
  Start();
}

HttpServerImpl::~HttpServerImpl() {
}

void HttpServerImpl::AddBinding(mojo::InterfaceRequest<HttpServer> request) {
  bindings_.AddBinding(this, request.Pass());
}

void HttpServerImpl::SetHandler(const mojo::String& path,
                                mojo::InterfaceHandle<HttpHandler> http_handler,
                                const mojo::Callback<void(bool)>& callback) {
  for (const auto& handler : handlers_) {
    if (handler->pattern->pattern() == path)
      callback.Run(false);
  }

  Handler* handler =
      new Handler(path, HttpHandlerPtr::Create(std::move(http_handler)));
  handler->http_handler.set_connection_error_handler(
      [this, handler]() { OnHandlerConnectionError(handler); });
  handlers_.push_back(handler);
  callback.Run(true);
}

void HttpServerImpl::GetPort(const GetPortCallback& callback) {
  if (assigned_port_)
    callback.Run(assigned_port_);
  else
    pending_get_port_callbacks_.push_back(callback);
}

void HttpServerImpl::OnHandlerConnectionError(Handler* handler) {
  auto it = std::find(handlers_.begin(), handlers_.end(), handler);
  CHECK(it != handlers_.end());
  DCHECK((*it)->http_handler.encountered_error());
  handlers_.erase(it);

  if (handlers_.empty()) {
    // The call deregisters the server from the factory and deletes |this|.
    factory_->DeleteServer(this, requested_local_address_.get());
  }
}

void HttpServerImpl::OnSocketBound(mojo::NetworkErrorPtr err,
                                   mojo::NetAddressPtr bound_address) {
  if (err->code != 0) {
    LOG(ERROR) << "Failed to bind the socket, err = " << err->code;
    return;
  }

  assigned_port_ = bound_address->ipv4->port;

  for (GetPortCallback& port_callback : pending_get_port_callbacks_) {
    port_callback.Run(assigned_port_);
  }
  pending_get_port_callbacks_.clear();
}

void HttpServerImpl::OnSocketListening(mojo::NetworkErrorPtr err) {
  if (err->code != 0) {
    LOG(ERROR) << "Failed to listen on the socket, err = " << err->code;
    return;
  }
}

void HttpServerImpl::OnConnectionAccepted(mojo::NetworkErrorPtr err,
                                          mojo::NetAddressPtr remote_address) {
  if (err->code != 0) {
    LOG(ERROR) << "Failed to accept a connection, err = " << err->code;
    return;
  }

  // Connection manages its own lifetime and can outlive the server, hence
  // binding a weak pointer.
  new Connection(pending_connected_socket_.Pass(), pending_send_handle_.Pass(),
                 pending_receive_handle_.Pass(),
                 base::Bind(&HttpServerImpl::HandleRequest,
                            weak_ptr_factory_.GetWeakPtr()));

  // Ready for another connection.
  WaitForNextConnection();
}

void HttpServerImpl::WaitForNextConnection() {
  // Need two pipes (one for each direction).
  mojo::ScopedDataPipeConsumerHandle send_consumer_handle;
  MojoResult result =
      CreateDataPipe(nullptr, &pending_send_handle_, &send_consumer_handle);
  assert(result == MOJO_RESULT_OK);

  mojo::ScopedDataPipeProducerHandle receive_producer_handle;
  result = CreateDataPipe(nullptr, &receive_producer_handle,
                          &pending_receive_handle_);
  assert(result == MOJO_RESULT_OK);
  MOJO_ALLOW_UNUSED_LOCAL(result);

  server_socket_->Accept(send_consumer_handle.Pass(),
                         receive_producer_handle.Pass(),
                         GetProxy(&pending_connected_socket_),
                         base::Bind(&HttpServerImpl::OnConnectionAccepted,
                                    base::Unretained(this)));
}

void HttpServerImpl::Start() {
  // Note that we can start using the proxies right away even thought the
  // callbacks have not been called yet. If a previous step fails, they'll
  // all fail.
  network_service_->CreateTCPBoundSocket(
      requested_local_address_.Clone(), GetProxy(&bound_socket_),
      base::Bind(&HttpServerImpl::OnSocketBound, base::Unretained(this)));
  bound_socket_->StartListening(
      GetProxy(&server_socket_),
      base::Bind(&HttpServerImpl::OnSocketListening, base::Unretained(this)));
  WaitForNextConnection();
}

void HttpServerImpl::HandleRequest(Connection* connection,
                                   HttpRequestPtr request) {
  for (auto& handler : handlers_) {
    if (RE2::FullMatch(request->relative_url.data(), *handler->pattern)) {
      handler->http_handler->HandleRequest(
          request.Pass(), base::Bind(&HttpServerImpl::OnResponse,
                                     base::Unretained(this), connection));
      return;
    }
  }

  connection->SendResponse(CreateHttpResponse(404, "No registered handler\n"));
}

void HttpServerImpl::OnResponse(Connection* connection,
                                HttpResponsePtr response) {
  connection->SendResponse(response.Pass());
}

HttpServerImpl::Handler::Handler(const std::string& pattern,
                                 HttpHandlerPtr http_handler)
    : pattern(new RE2(pattern.c_str())), http_handler(http_handler.Pass()) {
}

HttpServerImpl::Handler::~Handler() {
}

}  // namespace http_server
