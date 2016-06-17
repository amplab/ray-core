// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DART_CONTENT_HANDLER_APP_H_
#define SERVICES_DART_CONTENT_HANDLER_APP_H_

#include "base/bind.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/services/url_response_disk_cache/interfaces/url_response_disk_cache.mojom.h"
#include "services/dart/content_handler_app_service_connector.h"
#include "services/dart/dart_tracing.h"

namespace dart {

class DartContentHandlerApp;

class DartContentHandler : public mojo::ContentHandlerFactory::ManagedDelegate {
 public:
  DartContentHandler(DartContentHandlerApp* app, bool strict);

  void set_handler_task_runner(
      scoped_refptr<base::SingleThreadTaskRunner> handler_task_runner);

 private:
  // Overridden from ContentHandlerFactory::ManagedDelegate:
  std::unique_ptr<mojo::ContentHandlerFactory::HandledApplicationHolder>
  CreateApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override;

  DartContentHandlerApp* app_;
  bool strict_;
  scoped_refptr<base::SingleThreadTaskRunner> handler_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(DartContentHandler);
};

class DartContentHandlerApp : public mojo::ApplicationImplBase {
 public:
  DartContentHandlerApp();

  ~DartContentHandlerApp() override;

  void ExtractApplication(base::FilePath* application_dir,
                          mojo::URLResponsePtr response,
                          const base::Closure& callback);

  bool run_on_message_loop() const;

 private:
  // Overridden from mojo::ApplicationImplBase:
  void OnInitialize() override;
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override;

  mojo::TracingImpl tracing_;
  DartContentHandler content_handler_;
  DartContentHandler strict_content_handler_;
  mojo::URLResponseDiskCachePtr url_response_disk_cache_;
  ContentHandlerAppServiceConnector* service_connector_;
  DartTracingImpl dart_tracing_;
  bool default_strict_;
  bool run_on_message_loop_;

  DISALLOW_COPY_AND_ASSIGN(DartContentHandlerApp);
};

}  // namespace dart

#endif  // SERVICES_DART_CONTENT_HANDLER_APP_H_
