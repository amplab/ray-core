// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "services/clipboard/clipboard_standalone_impl.h"

namespace {

class ClipboardApp : public mojo::ApplicationImplBase {
 public:
  ClipboardApp() {}
  ~ClipboardApp() override {}

  // mojo::ApplicationImplBase override.
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<mojo::Clipboard>(
        [](const mojo::ConnectionContext& connection_context,
           mojo::InterfaceRequest<mojo::Clipboard> clipboard_request) {
          // TODO(erg): Write native implementations of the clipboard. For now,
          // we just build a clipboard which doesn't interact with the system.
          new clipboard::ClipboardStandaloneImpl(clipboard_request.Pass());
        });
    return true;
  }
};

}  // namespace

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  ClipboardApp clipboard_app;
  return mojo::RunApplication(application_request, &clipboard_app);
}
