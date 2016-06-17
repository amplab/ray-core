// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SHELL_ANDROID_NATIVE_VIEWPORT_APPLICATION_LOADER_H_
#define MOJO_SHELL_ANDROID_NATIVE_VIEWPORT_APPLICATION_LOADER_H_

#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/services/gpu/interfaces/gpu.mojom.h"
#include "mojo/services/native_viewport/interfaces/native_viewport.mojom.h"
#include "services/gles2/gpu_impl.h"
#include "shell/application_manager/application_loader.h"

namespace gles2 {
class GpuState;
}

namespace shell {

class NativeViewportApplicationLoader : public ApplicationLoader,
                                        public mojo::ApplicationImplBase {
 public:
  NativeViewportApplicationLoader();
  ~NativeViewportApplicationLoader();

 private:
  // ApplicationLoader implementation.
  void Load(
      const GURL& url,
      mojo::InterfaceRequest<mojo::Application> application_request) override;

  // mojo::ApplicationImplBase override.
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override;

  scoped_refptr<gles2::GpuState> gpu_state_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewportApplicationLoader);
};

}  // namespace shell

#endif  // MOJO_SHELL_ANDROID_NATIVE_VIEWPORT_APPLICATION_LOADER_H_
