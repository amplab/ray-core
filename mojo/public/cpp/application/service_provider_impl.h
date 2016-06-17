// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_APPLICATION_SERVICE_PROVIDER_IMPL_H_
#define MOJO_PUBLIC_APPLICATION_SERVICE_PROVIDER_IMPL_H_

#include <functional>
#include <map>
#include <string>
#include <utility>

#include "mojo/public/cpp/application/connection_context.h"
#include "mojo/public/cpp/application/service_connector.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"

namespace mojo {

// An implementation of |ServiceProvider|, which can be customized appropriately
// (to select what services it provides).
class ServiceProviderImpl : public ServiceProvider {
 public:
  // A |InterfaceRequestHandler<Interface>| is simply a function that handles an
  // interface request for |Interface|. If it determines (e.g., based on the
  // given |ConnectionContext|) that the request should be "accepted", then it
  // should "connect" ("take ownership of") request. Otherwise, it can simply
  // drop |interface_request| (as implied  by the interface).
  template <typename Interface>
  using InterfaceRequestHandler =
      std::function<void(const ConnectionContext& connection_context,
                         InterfaceRequest<Interface> interface_request)>;

  // Constructs this service provider implementation in an unbound state.
  ServiceProviderImpl();

  // Constructs this service provider implementation, binding it to the given
  // interface request. Note: If |service_provider_request| is not valid
  // ("pending"), then the object will be put into an unbound state.
  explicit ServiceProviderImpl(
      const ConnectionContext& connection_context,
      InterfaceRequest<ServiceProvider> service_provider_request);

  ~ServiceProviderImpl() override;

  // Binds this service provider implementation to the given interface request.
  // This may only be called if this object is unbound.
  void Bind(const ConnectionContext& connection_context,
            InterfaceRequest<ServiceProvider> service_provider_request);

  // Disconnect this service provider implementation and put it in a state where
  // it can be rebound to a new request (i.e., restores this object to an
  // unbound state). This may be called even if this object is already unbound.
  void Close();

  // Adds a supported service with the given |service_name|, using the given
  // |service_connector|.
  void AddServiceForName(std::unique_ptr<ServiceConnector> service_connector,
                         const std::string& service_name);

  // Adds a supported service with the given |service_name|, using the given
  // |interface_request_handler| (see above for information about
  // |InterfaceRequestHandler<Interface>|). |interface_request_handler| should
  // remain valid for the lifetime of this object.
  //
  // A typical usage may be:
  //
  //   service_provider_impl_->AddService<Foobar>(
  //       [](const ConnectionContext& connection_context,
  //          InterfaceRequest<FooBar> foobar_request) {
  //         // |FoobarImpl| owns itself.
  //         new FoobarImpl(std::move(foobar_request));
  //       });
  template <typename Interface>
  void AddService(InterfaceRequestHandler<Interface> interface_request_handler,
                  const std::string& service_name = Interface::Name_) {
    AddServiceForName(
        std::unique_ptr<ServiceConnector>(new ServiceConnectorImpl<Interface>(
            std::move(interface_request_handler))),
        service_name);
  }

  // Removes support for the service with the given |service_name|.
  void RemoveServiceForName(const std::string& service_name);

  // Like |RemoveServiceForName()| (above), but designed so that it can be used
  // like |RemoveService<Interface>()| or even
  // |RemoveService<Interface>(service_name)| (to parallel
  // |AddService<Interface>()|).
  template <typename Interface>
  void RemoveService(const std::string& service_name = Interface::Name_) {
    RemoveServiceForName(service_name);
  }

  // This uses the provided |fallback_service_provider| for connection requests
  // for services that are not known (haven't been added). (Set it to null to
  // not have any fallback.) A fallback must outlive this object (or until it is
  // "cleared" or replaced by a different fallback.
  void set_fallback_service_provider(
      ServiceProvider* fallback_service_provider) {
    fallback_service_provider_ = fallback_service_provider;
  }

  // Gets the context for the connection that this object is bound to (if not
  // bound, the context is just a default-initialized |ConnectionContext|).
  const ConnectionContext& connection_context() const {
    return connection_context_;
  }

 private:
  // Objects of this class are used to adapt a generic (untyped) connection
  // request (i.e., |ServiceConnector::ConnectToService()|) to the type-safe
  // |InterfaceRequestHandler<Interface>|.
  template <typename Interface>
  class ServiceConnectorImpl : public ServiceConnector {
   public:
    explicit ServiceConnectorImpl(
        InterfaceRequestHandler<Interface> interface_request_handler)
        : interface_request_handler_(std::move(interface_request_handler)) {}
    ~ServiceConnectorImpl() override {}

    void ConnectToService(const mojo::ConnectionContext& connection_context,
                          ScopedMessagePipeHandle client_handle) override {
      interface_request_handler_(
          connection_context,
          InterfaceRequest<Interface>(client_handle.Pass()));
    }

   private:
    const InterfaceRequestHandler<Interface> interface_request_handler_;

    MOJO_DISALLOW_COPY_AND_ASSIGN(ServiceConnectorImpl);
  };

  // Overridden from |ServiceProvider|:
  void ConnectToService(const String& service_name,
                        ScopedMessagePipeHandle client_handle) override;

  ConnectionContext connection_context_;
  Binding<ServiceProvider> binding_;

  std::map<std::string, std::unique_ptr<ServiceConnector>>
      name_to_service_connector_;

  ServiceProvider* fallback_service_provider_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ServiceProviderImpl);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_APPLICATION_SERVICE_PROVIDER_IMPL_H_
