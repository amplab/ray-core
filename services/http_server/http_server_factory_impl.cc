// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/http_server/http_server_factory_impl.h"

#include "base/stl_util.h"
#include "mojo/services/http_server/interfaces/http_server.mojom.h"
#include "services/http_server/http_server_impl.h"

namespace http_server {

HttpServerFactoryImpl::HttpServerFactoryImpl(mojo::Shell* shell)
    : shell_(shell) {}

HttpServerFactoryImpl::~HttpServerFactoryImpl() {
  // Free the http servers.
  STLDeleteContainerPairSecondPointers(port_indicated_servers_.begin(),
                                       port_indicated_servers_.end());
  STLDeleteContainerPointers(port_any_servers_.begin(),
                             port_any_servers_.end());
}

void HttpServerFactoryImpl::AddBinding(
    mojo::InterfaceRequest<HttpServerFactory> request) {
  bindings_.AddBinding(this, request.Pass());
}

void HttpServerFactoryImpl::DeleteServer(HttpServerImpl* server,
                                         mojo::NetAddress* requested_address) {
  ServerKey key = GetServerKey(requested_address);

  if (key.second) {  // If the port is non-zero.
    DCHECK(port_indicated_servers_.count(key));
    DCHECK_EQ(server, port_indicated_servers_[key]);

    delete server;
    port_indicated_servers_.erase(key);
  } else {
    DCHECK(port_any_servers_.count(server));

    delete server;
    port_any_servers_.erase(server);
  }
}

HttpServerFactoryImpl::ServerKey HttpServerFactoryImpl::GetServerKey(
    mojo::NetAddress* local_address) {
  DCHECK(local_address);

  if (local_address->family == mojo::NetAddressFamily::IPV6) {
    return ServerKey(local_address->ipv6->addr, local_address->ipv6->port);
  } else if (local_address->family == mojo::NetAddressFamily::IPV4) {
    return ServerKey(local_address->ipv4->addr, local_address->ipv4->port);
  } else {
    return ServerKey();
  }
}

void HttpServerFactoryImpl::CreateHttpServer(
    mojo::InterfaceRequest<HttpServer> server_request,
    mojo::NetAddressPtr local_address) {
  if (!local_address) {
    local_address = mojo::NetAddress::New();
    local_address->family = mojo::NetAddressFamily::IPV4;
    local_address->ipv4 = mojo::NetAddressIPv4::New();
    local_address->ipv4->addr.resize(4);
    local_address->ipv4->addr[0] = 0;
    local_address->ipv4->addr[1] = 0;
    local_address->ipv4->addr[2] = 0;
    local_address->ipv4->addr[3] = 0;
    local_address->ipv4->port = 0;
  }
  ServerKey key = GetServerKey(local_address.get());

  if (key.second) {  // If the port is non-zero.
    if (!port_indicated_servers_.count(key)) {
      port_indicated_servers_[key] =
          new HttpServerImpl(shell_, this, local_address.Pass());
    }
    port_indicated_servers_[key]->AddBinding(server_request.Pass());
  } else {
    HttpServerImpl* server =
        new HttpServerImpl(shell_, this, local_address.Pass());
    server->AddBinding(server_request.Pass());
    port_any_servers_.insert(server);
  }
}

}  // namespace http_server
