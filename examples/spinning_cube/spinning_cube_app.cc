// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "examples/spinning_cube/gles2_client_impl.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/services/native_viewport/interfaces/native_viewport.mojom.h"
#include "mojo/services/native_viewport/interfaces/native_viewport_event_dispatcher.mojom.h"

namespace examples {

class SpinningCubeApp : public mojo::ApplicationImplBase,
                        public mojo::NativeViewportEventDispatcher {
 public:
  SpinningCubeApp() : dispatcher_binding_(this) {}

  ~SpinningCubeApp() override {
    // TODO(darin): Fix shutdown so we don't need to leak this.
    mojo::ignore_result(gles2_client_.release());
  }

  // |ApplicationImplBase| overrides:
  void OnInitialize() override {
    mojo::ConnectToService(shell(), "mojo:native_viewport_service",
                           GetProxy(&viewport_));
    viewport_.set_connection_error_handler(
        [this]() { OnViewportConnectionError(); });

    SetEventDispatcher();

    mojo::SizePtr size(mojo::Size::New());
    size->width = 800;
    size->height = 600;

    auto requested_configuration = mojo::SurfaceConfiguration::New();
    requested_configuration->depth_bits = 16;

    viewport_->Create(
        size.Clone(),
        requested_configuration.Pass(),
        base::Bind(&SpinningCubeApp::OnMetricsChanged, base::Unretained(this)));
    viewport_->Show();
    mojo::ContextProviderPtr onscreen_context_provider;
    viewport_->GetContextProvider(GetProxy(&onscreen_context_provider));

    gles2_client_.reset(new GLES2ClientImpl(onscreen_context_provider.Pass()));
    gles2_client_->SetSize(*size);
  }

  void OnMetricsChanged(mojo::ViewportMetricsPtr metrics) {
    assert(metrics);
    gles2_client_->SetSize(*metrics->size);
    viewport_->RequestMetrics(
        base::Bind(&SpinningCubeApp::OnMetricsChanged, base::Unretained(this)));
  }

  void OnEvent(mojo::EventPtr event,
               const mojo::Callback<void()>& callback) override {
    assert(event);
    if (event->pointer_data.get())
      gles2_client_->HandleInputEvent(*event);
    callback.Run();
  }

 private:
  void SetEventDispatcher() {
    mojo::InterfaceHandle<mojo::NativeViewportEventDispatcher> ptr;
    dispatcher_binding_.Bind(GetProxy(&ptr));
    viewport_->SetEventDispatcher(std::move(ptr));
  }

  void OnViewportConnectionError() { mojo::RunLoop::current()->Quit(); }

  std::unique_ptr<GLES2ClientImpl> gles2_client_;
  mojo::NativeViewportPtr viewport_;
  mojo::ContextProviderPtr onscreen_context_provider_;
  mojo::Binding<NativeViewportEventDispatcher> dispatcher_binding_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(SpinningCubeApp);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  examples::SpinningCubeApp spinning_cube_app;
  return mojo::RunApplication(application_request, &spinning_cube_app);
}
