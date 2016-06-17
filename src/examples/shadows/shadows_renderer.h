// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_SHADOWS_SHADOWS_RENDERER_H_
#define EXAMPLES_SHADOWS_SHADOWS_RENDERER_H_

#include "base/macros.h"
#include "examples/shadows/penumbra_program.h"
#include "examples/shadows/vfx/color_program.h"
#include "examples/shadows/vfx/element_array_buffer.h"
#include "mojo/services/geometry/interfaces/geometry.mojom.h"

namespace examples {

class ShadowsRenderer {
 public:
  ShadowsRenderer();
  ~ShadowsRenderer();

  void Render(const mojo::Size& size);

 private:
  vfx::ColorProgram color_program_;
  PenumbraProgram penumbra_program_;
  vfx::ElementArrayBuffer<vfx::ColorProgram::Vertex> shapes_;
  vfx::ElementArrayBuffer<PenumbraProgram::Vertex> penumbra_;

  DISALLOW_COPY_AND_ASSIGN(ShadowsRenderer);
};

}  // namespace examples

#endif  // EXAMPLES_SHADOWS_SHADOWS_RENDERER_H_
