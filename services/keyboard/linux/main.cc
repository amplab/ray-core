// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/native_viewport/interfaces/native_viewport_event_dispatcher.mojom.h"
#include "services/keyboard/linux/keyboard_service_impl.h"

namespace keyboard {

class KeyboardServiceFactoryImpl : public keyboard::KeyboardServiceFactory {
public:
  explicit KeyboardServiceFactoryImpl(
      mojo::InterfaceRequest<KeyboardServiceFactory> request)
    : binding_(this, request.Pass()) {}

  // |keyboard::KeyboardServiceFactory| implementation:
  void CreateKeyboardService(
      mojo::InterfaceRequest<mojo::NativeViewportEventDispatcher> dispatcher,
      mojo::InterfaceRequest<KeyboardService> request) override {
    new LinuxKeyboardServiceImpl(request.Pass(), dispatcher.Pass());
  }

private:
  mojo::StrongBinding<keyboard::KeyboardServiceFactory> binding_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardServiceFactoryImpl);
};

class KeyboardServiceApp : public mojo::ApplicationImplBase {
 public:
  KeyboardServiceApp() {}
  ~KeyboardServiceApp() override {}

 private:
  // |ApplicationImplBase| override:
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<KeyboardServiceFactory>([](
        const mojo::ConnectionContext& connection_context,
        mojo::InterfaceRequest<KeyboardServiceFactory>
            keyboard_service_factory_request) {
      new KeyboardServiceFactoryImpl(keyboard_service_factory_request.Pass());
    });
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardServiceApp);
};

}  // namespace keyboard

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  keyboard::KeyboardServiceApp keyboard_service_app;
  return mojo::RunApplication(application_request, &keyboard_service_app);
}
