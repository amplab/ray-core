// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include <utility>

#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/log/interfaces/log.mojom.h"
#include "services/log/log_impl.h"

namespace mojo {
namespace log {

// Provides the mojo.log.Log service.  Binds a new Log implementation for each
// Log interface request.
class LogApp : public ApplicationImplBase {
 public:
  LogApp() {}
  ~LogApp() override {}

 private:
  // |ApplicationImplBase| override:
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<Log>(
        [](const ConnectionContext& connection_context,
           InterfaceRequest<Log> log_request) {
          LogImpl::Create(connection_context, std::move(log_request),
                          [](const std::string& message) {
                            fprintf(stderr, "%s\n", message.c_str());
                          });
        });
    return true;
  }

  MOJO_DISALLOW_COPY_AND_ASSIGN(LogApp);
};

}  // namespace log
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  mojo::log::LogApp log_app;
  return mojo::RunApplication(application_request, &log_app);
}
