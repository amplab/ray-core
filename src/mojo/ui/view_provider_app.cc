// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/ui/view_provider_app.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "mojo/public/cpp/application/service_provider_impl.h"

namespace mojo {
namespace ui {

class ViewProviderApp::DelegatingViewProvider : public mojo::ui::ViewProvider {
 public:
  DelegatingViewProvider(ViewProviderApp* app,
                         const std::string& view_provider_url)
      : app_(app), view_provider_url_(view_provider_url) {}

  ~DelegatingViewProvider() override {}

 private:
  // |ViewProvider|:
  void CreateView(
      mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
      mojo::InterfaceRequest<mojo::ServiceProvider> services) override {
    app_->CreateView(this, view_provider_url_, view_owner_request.Pass(),
                     services.Pass());
  }

  ViewProviderApp* app_;
  std::string view_provider_url_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(DelegatingViewProvider);
};

ViewProviderApp::ViewProviderApp() {}

ViewProviderApp::~ViewProviderApp() {}

void ViewProviderApp::OnInitialize() {
  auto command_line = base::CommandLine::ForCurrentProcess();
  command_line->InitFromArgv(args());
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);

  tracing_.Initialize(shell(), &args());
}

bool ViewProviderApp::OnAcceptConnection(
    ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<ViewProvider>(
      [this](const ConnectionContext& connection_context,
             InterfaceRequest<ViewProvider> view_provider_request) {
        bindings_.AddBinding(
            new DelegatingViewProvider(this, connection_context.connection_url),
            view_provider_request.Pass());
      });
  return true;
}

void ViewProviderApp::CreateView(
    DelegatingViewProvider* provider,
    const std::string& view_provider_url,
    mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
    mojo::InterfaceRequest<mojo::ServiceProvider> services) {
  CreateView(view_provider_url, view_owner_request.Pass(), services.Pass());
}

}  // namespace ui
}  // namespace mojo
