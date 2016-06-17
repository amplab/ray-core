// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_HTTP_SERVER_CONNECTION_H_
#define SERVICES_HTTP_SERVER_CONNECTION_H_

#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/public/cpp/environment/async_waiter.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/services/http_server/interfaces/http_request.mojom.h"
#include "mojo/services/http_server/interfaces/http_response.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "services/http_server/http_request_parser.h"

namespace http_server {

// Represents one connection to a client. This connection will manage its own
// lifetime and will delete itself when the connection is closed.
class Connection {
 public:
  // Callback called when a request is parsed. Response should be sent
  // using Connection::SendResponse() on the |connection| argument.
  typedef base::Callback<void(Connection*, HttpRequestPtr)> Callback;

  Connection(mojo::TCPConnectedSocketPtr conn,
             mojo::ScopedDataPipeProducerHandle sender,
             mojo::ScopedDataPipeConsumerHandle receiver,
             const Callback& callback);

  ~Connection();

  void SendResponse(HttpResponsePtr response);

 private:
  // Called when we have more data available from the request.
  void OnRequestDataReady(MojoResult result);

  void WriteMore();

  void OnResponseDataReady(MojoResult result);

  void OnSenderReady(MojoResult result);

  mojo::TCPConnectedSocketPtr connection_;
  mojo::ScopedDataPipeProducerHandle sender_;
  mojo::ScopedDataPipeConsumerHandle receiver_;

  // Used to wait for the request data.
  mojo::AsyncWaiter request_waiter_;

  int content_length_;
  mojo::ScopedDataPipeConsumerHandle content_;

  // Used to wait for the response data to send.
  scoped_ptr<mojo::AsyncWaiter> response_receiver_waiter_;

  // Used to wait for the sender to be ready to accept more data.
  scoped_ptr<mojo::AsyncWaiter> sender_waiter_;

  HttpRequestParser request_parser_;

  // Callback to run once all of the request has been read.
  const Callback handle_request_callback_;

  // Contains response data to write to the pipe. Initially it is the headers,
  // and then when they're written it contains chunks of the body.
  std::string response_;
  size_t response_offset_;
};

}  // namespace http_server

#endif  // SERVICES_HTTP_SERVER_CONNECTION_H_
