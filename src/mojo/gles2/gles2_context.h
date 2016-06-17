// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_GLES2_GLES2_CONTEXT_H_
#define MOJO_GLES2_GLES2_CONTEXT_H_

#include <memory>

#include "base/macros.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "mojo/gles2/command_buffer_client_impl.h"
#include "mojo/public/c/gpu/MGL/mgl_echo.h"
#include "mojo/public/c/gpu/MGL/mgl_signal_sync_point.h"
#include "mojo/public/c/gpu/MGL/mgl_types.h"

struct MGLContextPrivate {};

namespace gpu {
class TransferBuffer;
namespace gles2 {
class GLES2CmdHelper;
class GLES2Implementation;
}
}

namespace gles2 {

class GLES2Context : public CommandBufferDelegate, public MGLContextPrivate {
 public:
  explicit GLES2Context(const MojoAsyncWaiter* async_waiter,
                        mojo::ScopedMessagePipeHandle command_buffer_handle,
                        MGLContextLostCallback lost_callback,
                        void* closure);
  ~GLES2Context() override;
  bool Initialize();

  gpu::gles2::GLES2Interface* interface() const {
    return implementation_.get();
  }
  gpu::ContextSupport* context_support() const { return implementation_.get(); }

  void Echo(MGLEchoCallback callback, void* closure);

  void SignalSyncPoint(uint32_t sync_point,
                       MGLSignalSyncPointCallback callback,
                       void* closure);

 private:
  void ContextLost() override;

  CommandBufferClientImpl command_buffer_;
  std::unique_ptr<gpu::gles2::GLES2CmdHelper> gles2_helper_;
  std::unique_ptr<gpu::TransferBuffer> transfer_buffer_;
  std::unique_ptr<gpu::gles2::GLES2Implementation> implementation_;
  MGLContextLostCallback lost_callback_;
  void* closure_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(GLES2Context);
};

}  // namespace gles2

#endif  // MOJO_GLES2_GLES2_CONTEXT_H_
