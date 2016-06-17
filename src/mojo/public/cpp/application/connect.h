// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Helpers for using |ServiceProvider|s.

#ifndef MOJO_PUBLIC_CPP_APPLICATION_CONNECT_H_
#define MOJO_PUBLIC_CPP_APPLICATION_CONNECT_H_

#include "mojo/public/cpp/bindings/interface_handle.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/interfaces/application/application_connector.mojom.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"

namespace mojo {

// Helper for using a |ServiceProvider|'s |ConnectToService()| that takes a
// fully-typed interface request (and can use the default interface name). Note
// that this can be used in conjunction with |GetProxy()|, etc. E.g.:
//
//   FooPtr foo;
//   ConnectToService(service_provider, GetProxy(&foo));
template <typename Interface>
inline void ConnectToService(
    ServiceProvider* service_provider,
    InterfaceRequest<Interface> interface_request,
    const std::string& interface_name = Interface::Name_) {
  service_provider->ConnectToService(interface_name,
                                     interface_request.PassMessagePipe());
}

// Helper for connecting to an application (specified by URL) using the |Shell|
// and then a service from it. (As above, this is typed and may be used with
// |GetProxy()|.)
template <typename Interface>
inline void ConnectToService(
    Shell* shell,
    const std::string& application_url,
    InterfaceRequest<Interface> interface_request,
    const std::string& interface_name = Interface::Name_) {
  ServiceProviderPtr service_provider;
  shell->ConnectToApplication(application_url, GetProxy(&service_provider));
  ConnectToService(service_provider.get(), interface_request.Pass(),
                   interface_name);
}

// Helper for connecting to an application (specified by URL) using an
// |ApplicationConnector| and then a service from it. (As above, this is typed
// and may be used with |GetProxy()|.)
template <typename Interface>
inline void ConnectToService(ApplicationConnector* application_connector,
                             const std::string& application_url,
                             InterfaceRequest<Interface> request) {
  ServiceProviderPtr service_provider;
  application_connector->ConnectToApplication(application_url,
                                              GetProxy(&service_provider));
  ConnectToService(service_provider.get(), request.Pass());
}

// Helper for getting an |InterfaceHandle<ApplicationConnector>| (which can be
// passed to any thread) from the shell.
InterfaceHandle<ApplicationConnector> CreateApplicationConnector(Shell* shell);

// Helper for "duplicating" a |ApplicationConnector| (typically, from an
// |ApplicationConnectorPtr|, getting another independent
// |InterfaceHandle<ApplicationConnector>|).
InterfaceHandle<ApplicationConnector> DuplicateApplicationConnector(
    ApplicationConnector* application_connector);

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_APPLICATION_CONNECT_H_
