// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_APPLICATION_CONNECTION_CONTEXT_H_
#define MOJO_PUBLIC_APPLICATION_CONNECTION_CONTEXT_H_

#include <string>

namespace mojo {

// A |ConnectionContext| is used to track context about a given message pipe
// connection. It is usually associated to the "impl" side of an interface.
//
// |Application| (see //mojo/public/interfaces/application/application.mojom)
// accepts a message that looks like:
//
//   AcceptConnection(string requestor_url,
//                    string resolved_url,
//                    ServiceProvider& services);
//
// Upon handling |AcceptConnection()|, the |ServiceProvider| implementation
// bound to |services| is given a |ConnectionContext| with |type =
// Type::INCOMING|, |remote_url = requestor_url|, and |connection_url =
// resolved_url|.
//
// The |ConnectionContext| is meant to be propagated: If the remote side uses
// its |ServiceProviderPtr| to request a |Foo| (from the local side), then the
// |Foo| implementation (again, on the local side) may be given (a copy of) the
// |ConnectionContext| described above.
struct ConnectionContext {
  enum class Type {
    UNKNOWN = 0,
    INCOMING,
  };

  ConnectionContext() {}
  ConnectionContext(Type type,
                    const std::string& remote_url,
                    const std::string& connection_url)
      : type(type), remote_url(remote_url), connection_url(connection_url) {}

  Type type = Type::UNKNOWN;

  // The URL of the remote side of this connection (as provided by the |Shell|);
  // if unknown/not applicable, this is empty.
  std::string remote_url;

  // The URL used by the remote side to establish "this" connection (or a parent
  // thereof); if unknown/not applicable, this is empty.
  std::string connection_url;
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_APPLICATION_CONNECTION_CONTEXT_H_
