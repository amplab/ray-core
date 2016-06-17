// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_APPLICATION_SERVICE_CONNECTOR_H_
#define MOJO_PUBLIC_APPLICATION_SERVICE_CONNECTOR_H_

#include <string>

#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {

struct ConnectionContext;

// |ServiceConnector| is the generic, type-unsafe interface for objects used by
// |ServiceProviderImpl| to connect generic "interface requests" (i.e., just
// message pipes) specified by service name to service implementations.
class ServiceConnector {
 public:
  virtual ~ServiceConnector() {}

  // Connects to the |ServiceConnector|'s service (if the |ServiceConnector|
  // wishes to refuse the connection, it should simply drop |handle|).
  virtual void ConnectToService(const ConnectionContext& connection_context,
                                ScopedMessagePipeHandle handle) = 0;
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_APPLICATION_SERVICE_CONNECTOR_H_
