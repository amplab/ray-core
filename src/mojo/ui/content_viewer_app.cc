// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/ui/content_viewer_app.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/ui/view_provider_app.h"

namespace mojo {
namespace ui {

class ContentViewerApp::DelegatingContentHandler : public mojo::ContentHandler {
 public:
  DelegatingContentHandler(ContentViewerApp* app,
                           const std::string& content_handler_url)
      : app_(app), content_handler_url_(content_handler_url) {}

  ~DelegatingContentHandler() override {}

 private:
  // |ContentHandler|:
  void StartApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override {
    app_->StartViewer(content_handler_url_, application_request.Pass(),
                      response.Pass());
  }

  ContentViewerApp* app_;
  std::string content_handler_url_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(DelegatingContentHandler);
};

ContentViewerApp::ContentViewerApp() {}

ContentViewerApp::~ContentViewerApp() {}

void ContentViewerApp::OnInitialize() {
  auto command_line = base::CommandLine::ForCurrentProcess();
  command_line->InitFromArgv(args());
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);
}

bool ContentViewerApp::OnAcceptConnection(
    ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<ContentHandler>([this](
      const ConnectionContext& connection_context,
      InterfaceRequest<ContentHandler> content_handler_request) {
    bindings_.AddBinding(
        new DelegatingContentHandler(this, connection_context.connection_url),
        content_handler_request.Pass());
  });
  return true;
}

void ContentViewerApp::StartViewer(
    const std::string& content_handler_url,
    mojo::InterfaceRequest<mojo::Application> application_request,
    mojo::URLResponsePtr response) {
  // TODO(vtl): This is usually leaky, since |*app| (the returned
  // |ApplicationImplBase|/|ViewProviderApp| implementation) typically doesn't
  // own itself. Probably |LoadContent()| should take the |application_request|,
  // and not return anything. This method doesn't really appear to add anything.
  ViewProviderApp* app = LoadContent(content_handler_url, response.Pass());
  if (app)
    app->Bind(application_request.Pass());
}

}  // namespace ui
}  // namespace mojo
