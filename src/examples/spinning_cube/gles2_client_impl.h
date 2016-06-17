// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_SPINNING_CUBE_GLES2_CLIENT_IMPL_H_
#define EXAMPLES_SPINNING_CUBE_GLES2_CLIENT_IMPL_H_

#include "examples/spinning_cube/spinning_cube.h"
#include "mojo/public/c/gpu/MGL/mgl.h"
#include "mojo/services/geometry/interfaces/geometry.mojom.h"
#include "mojo/services/native_viewport/interfaces/native_viewport.mojom.h"

namespace examples {

class GLES2ClientImpl {
 public:
  explicit GLES2ClientImpl(mojo::ContextProviderPtr context_provider);
  virtual ~GLES2ClientImpl();

  void SetSize(const mojo::Size& size);
  void HandleInputEvent(const mojo::Event& event);
  void Draw();

 private:
  void ContextCreated(
      mojo::InterfaceHandle<mojo::CommandBuffer> command_buffer);
  void ContextLost();
  static void ContextLostThunk(void* closure);
  void WantToDraw();

  MojoTimeTicks last_time_;
  mojo::Size size_;
  SpinningCube cube_;
  mojo::PointF capture_point_;
  mojo::PointF last_drag_point_;
  MojoTimeTicks drag_start_time_;
  bool waiting_to_draw_;

  mojo::ContextProviderPtr context_provider_;
  MGLContext context_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(GLES2ClientImpl);
};

}  // namespace examples

#endif  // EXAMPLES_SPINNING_CUBE_GLES2_CLIENT_IMPL_H_
