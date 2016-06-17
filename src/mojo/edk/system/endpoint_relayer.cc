// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/endpoint_relayer.h"

#include <utility>

#include "base/logging.h"
#include "mojo/edk/system/channel_endpoint.h"
#include "mojo/edk/system/message_in_transit.h"

using mojo::util::MutexLocker;
using mojo::util::RefPtr;

namespace mojo {
namespace system {

// static
unsigned EndpointRelayer::GetPeerPort(unsigned port) {
  DCHECK(port == 0 || port == 1);
  return port ^ 1;
}

void EndpointRelayer::Init(RefPtr<ChannelEndpoint>&& endpoint0,
                           RefPtr<ChannelEndpoint>&& endpoint1) {
  DCHECK(endpoint0);
  DCHECK(endpoint1);
  DCHECK(!endpoints_[0]);
  DCHECK(!endpoints_[1]);
  endpoints_[0] = std::move(endpoint0);
  endpoints_[1] = std::move(endpoint1);
}

void EndpointRelayer::SetFilter(std::unique_ptr<Filter> filter) {
  MutexLocker locker(&mutex_);
  filter_ = std::move(filter);
}

bool EndpointRelayer::OnReadMessage(unsigned port, MessageInTransit* message) {
  DCHECK(message);

  MutexLocker locker(&mutex_);

  // If we're no longer the client, then reject the message.
  if (!endpoints_[port])
    return false;

  unsigned peer_port = GetPeerPort(port);

  if (filter_ && message->type() == MessageInTransit::Type::ENDPOINT_CLIENT) {
    if (filter_->OnReadMessage(endpoints_[port].get(),
                               endpoints_[peer_port].get(), message))
      return true;
  }

  // Otherwise, consume it even if the peer port is closed.
  if (endpoints_[peer_port]) {
    endpoints_[peer_port]->EnqueueMessage(
        std::unique_ptr<MessageInTransit>(message));
  }
  return true;
}

void EndpointRelayer::OnDetachFromChannel(unsigned port) {
  MutexLocker locker(&mutex_);

  if (endpoints_[port]) {
    endpoints_[port]->DetachFromClient();
    endpoints_[port] = nullptr;
  }

  unsigned peer_port = GetPeerPort(port);
  if (endpoints_[peer_port]) {
    endpoints_[peer_port]->DetachFromClient();
    endpoints_[peer_port] = nullptr;
  }
}

EndpointRelayer::EndpointRelayer() {}

EndpointRelayer::~EndpointRelayer() {
  DCHECK(!endpoints_[0]);
  DCHECK(!endpoints_[1]);
}

}  // namespace system
}  // namespace mojo
