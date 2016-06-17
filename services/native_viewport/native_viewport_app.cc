// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_viewport/native_viewport_app.h"

#include <vector>

#include "gpu/config/gpu_util.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/native_viewport/cpp/args.h"
#include "services/gles2/gpu_impl.h"
#include "services/native_viewport/native_viewport_impl.h"
#include "ui/events/event_switches.h"
#include "ui/gl/gl_surface.h"

namespace native_viewport {

NativeViewportApp::NativeViewportApp() : is_headless_(false) {}

NativeViewportApp::~NativeViewportApp() {}

void NativeViewportApp::OnInitialize() {
  InitLogging();
  tracing_.Initialize(shell(), &args());

  // Apply the switch for kTouchEvents to CommandLine (if set). This allows
  // redirecting the mouse to a touch device on X for testing.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  const std::string touch_event_string("--" +
                                       std::string(switches::kTouchDevices));
  auto touch_iter = std::find(args().begin(), args().end(), touch_event_string);
  if (touch_iter != args().end() && ++touch_iter != args().end())
    command_line->AppendSwitchASCII(touch_event_string, *touch_iter);

  gpu::ApplyGpuDriverBugWorkarounds(command_line);

  if (HasArg(mojo::kUseTestConfig))
    gfx::GLSurface::InitializeOneOffForTests();
  else if (HasArg(mojo::kUseOSMesa))
    gfx::GLSurface::InitializeOneOff(gfx::kGLImplementationOSMesaGL);
  else
    gfx::GLSurface::InitializeOneOff();

  is_headless_ = HasArg(mojo::kUseHeadlessConfig);
}

bool NativeViewportApp::OnAcceptConnection(
    mojo::ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<mojo::NativeViewport>([this](
      const mojo::ConnectionContext& connection_context,
      mojo::InterfaceRequest<mojo::NativeViewport> native_viewport_request) {
    if (!gpu_state_.get())
      gpu_state_ = new gles2::GpuState();
    new NativeViewportImpl(shell(), is_headless_, gpu_state_,
                           native_viewport_request.Pass());
  });

  service_provider_impl->AddService<mojo::Gpu>(
      [this](const mojo::ConnectionContext& connection_context,
             mojo::InterfaceRequest<mojo::Gpu> gpu_request) {
        if (!gpu_state_.get())
          gpu_state_ = new gles2::GpuState();
        new gles2::GpuImpl(gpu_request.Pass(), gpu_state_);
      });

  return true;
}

void NativeViewportApp::InitLogging() {
  std::vector<const char*> argv;
  for (const auto& a : args())
    argv.push_back(a.c_str());

  base::CommandLine::Reset();
  base::CommandLine::Init(argv.size(), argv.data());

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);
}

}  // namespace native_viewport
