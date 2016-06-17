// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_VIEWPORT_APP_H_
#define SERVICES_NATIVE_VIEWPORT_APP_H_

#include "base/macros.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "services/gles2/gpu_state.h"

namespace native_viewport {

class NativeViewportApp : public mojo::ApplicationImplBase {
 public:
  NativeViewportApp();
  ~NativeViewportApp() override;

  // mojo::ApplicationImplBase overrides.
  void OnInitialize() override;
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override;

 private:
  void InitLogging();

  scoped_refptr<gles2::GpuState> gpu_state_;
  bool is_headless_;
  mojo::TracingImpl tracing_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewportApp);
};

}  // namespace native_viewport

#endif  // SERVICES_NATIVE_VIEWPORT_APP_H_
