// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/native_viewport_application_loader.h"

#include "mojo/public/cpp/application/service_provider_impl.h"
#include "services/gles2/gpu_state.h"
#include "services/native_viewport/native_viewport_impl.h"

using mojo::ConnectionContext;
using mojo::InterfaceRequest;

namespace shell {

NativeViewportApplicationLoader::NativeViewportApplicationLoader() {}

NativeViewportApplicationLoader::~NativeViewportApplicationLoader() {}

void NativeViewportApplicationLoader::Load(
    const GURL& url,
    InterfaceRequest<mojo::Application> application_request) {
  DCHECK(application_request.is_pending());
  Bind(application_request.Pass());
}

bool NativeViewportApplicationLoader::OnAcceptConnection(
    mojo::ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<mojo::NativeViewport>(
      [this](const ConnectionContext& connection_context,
             InterfaceRequest<mojo::NativeViewport> native_viewport_request) {
        if (!gpu_state_)
          gpu_state_ = new gles2::GpuState();
        new native_viewport::NativeViewportImpl(shell(), false, gpu_state_,
                                                native_viewport_request.Pass());
      });
  service_provider_impl->AddService<mojo::Gpu>(
      [this](const ConnectionContext& connection_context,
             InterfaceRequest<mojo::Gpu> gpu_request) {
        if (!gpu_state_)
          gpu_state_ = new gles2::GpuState();
        new gles2::GpuImpl(gpu_request.Pass(), gpu_state_);
      });
  return true;
}

}  // namespace shell
