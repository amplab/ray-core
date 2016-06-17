// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "examples/echo/echo.mojom.h"
#include "mojo/common/binding_set.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace mojo {
namespace examples {

// This file demonstrates three ways to implement a simple Mojo server. Because
// there's no state that's saved per-pipe, all three servers appear to the
// client to do exactly the same thing.
//
// To choose one of the them, update MojoMain() at the end of this file.
// 1. MultiServer - creates a new object for each connection. Cleans up by using
//    StrongBinding.
// 2. SingletonServer -- all requests are handled by one object. Connections are
//    tracked using BindingSet.
// 3. OneAtATimeServer -- each request "replaces" any previous request. Any old
//    interface pipe is closed and the new interface pipe is bound.

// EchoImpl defines our implementation of the Echo interface.
// It is used by all three server variants.
class EchoImpl : public Echo {
 public:
  void EchoString(const mojo::String& value,
                  const Callback<void(mojo::String)>& callback) override {
    callback.Run(value);
  }
};

// StrongBindingEchoImpl inherits from EchoImpl and adds the StrongBinding<>
// class so instances will delete themselves when the message pipe is closed.
// This simplifies lifetime management. This class is only used by MultiServer.
class StrongBindingEchoImpl : public EchoImpl {
 public:
  explicit StrongBindingEchoImpl(InterfaceRequest<Echo> handle)
      : strong_binding_(this, handle.Pass()) {}

 private:
  mojo::StrongBinding<Echo> strong_binding_;
};

// MultiServer creates a new object to handle each message pipe.
class MultiServer : public mojo::ApplicationImplBase {
 public:
  MultiServer() {}

  // From ApplicationImplBase
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<Echo>(
        [](const mojo::ConnectionContext& connection_context,
           mojo::InterfaceRequest<Echo> echo_request) {
          // This object will be deleted automatically because of the use of
          // StrongBinding<> for the declaration of |strong_binding_|.
          new StrongBindingEchoImpl(echo_request.Pass());
        });
    return true;
  }
};

// SingletonServer uses the same object to handle all message pipes. Useful
// for stateless operation.
class SingletonServer : public mojo::ApplicationImplBase {
 public:
  SingletonServer() {}

  // From ApplicationImplBase
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<Echo>(
        [this](const mojo::ConnectionContext& connection_context,
               mojo::InterfaceRequest<Echo> echo_request) {
          // All channels will connect to this singleton object, so just
          // add the binding to our collection.
          bindings_.AddBinding(&echo_impl_, echo_request.Pass());
        });
    return true;
  }

 private:
  EchoImpl echo_impl_;

  mojo::BindingSet<Echo> bindings_;
};

// OneAtATimeServer works with only one pipe at a time. When a new pipe tries
// to bind, the previous pipe is closed. This would seem to be useful when
// clients are expected to make a single call and then go away, but in fact it's
// not reliable. There's a race condition because a second client could bind
// to the server before the first client called EchoString(). Therefore, this
// is an example of how not to write your code.
class OneAtATimeServer : public mojo::ApplicationImplBase {
 public:
  OneAtATimeServer() : binding_(&echo_impl_) {}

  // From ApplicationImplBase
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<Echo>(
        [this](const mojo::ConnectionContext& connection_context,
               mojo::InterfaceRequest<Echo> echo_request) {
          binding_.Bind(echo_request.Pass());
        });
    return true;
  }

 private:
  EchoImpl echo_impl_;

  mojo::Binding<Echo> binding_;
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  // Uncomment one of the three servers at a time to see it work:
  mojo::examples::MultiServer server_app;
  // mojo::examples::SingletonServer server_app;
  // mojo::examples::OneAtATimeServer server_app;
  return mojo::RunApplication(application_request, &server_app);
}
