// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_GLES2_GPU_IMPL_H_
#define SERVICES_GLES2_GPU_IMPL_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/geometry/interfaces/geometry.mojom.h"
#include "mojo/services/gpu/interfaces/command_buffer.mojom.h"
#include "mojo/services/gpu/interfaces/gpu.mojom.h"
#include "services/gles2/gpu_state.h"

namespace gfx {
class GLShareGroup;
}

namespace gpu {
class SyncPointManager;
namespace gles2 {
class MailboxManager;
}
}

namespace gles2 {

class GpuImpl : public mojo::Gpu {
 public:
  GpuImpl(mojo::InterfaceRequest<mojo::Gpu> request,
          const scoped_refptr<GpuState>& state);
  ~GpuImpl() override;

 private:
  void CreateOffscreenGLES2Context(mojo::InterfaceRequest<mojo::CommandBuffer>
                                       command_buffer_request) override;

  mojo::StrongBinding<Gpu> binding_;
  scoped_refptr<GpuState> state_;

  DISALLOW_COPY_AND_ASSIGN(GpuImpl);
};

}  // namespace gles2

#endif  // SERVICES_GLES2_GPU_IMPL_H_
