// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/gfx/compositor/compositor_app.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "services/gfx/compositor/compositor_impl.h"

namespace compositor {

CompositorApp::CompositorApp() {}

CompositorApp::~CompositorApp() {}

void CompositorApp::OnInitialize() {
  auto command_line = base::CommandLine::ForCurrentProcess();
  command_line->InitFromArgv(args());
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);

  tracing_.Initialize(shell(), &args());

  engine_.reset(new CompositorEngine());
}

bool CompositorApp::OnAcceptConnection(
    mojo::ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<mojo::gfx::composition::Compositor>(
      [this](const mojo::ConnectionContext& connection_context,
             mojo::InterfaceRequest<mojo::gfx::composition::Compositor>
                 compositor_request) {
        compositor_bindings_.AddBinding(new CompositorImpl(engine_.get()),
                                        compositor_request.Pass());
      });
  return true;
}

}  // namespace compositor
