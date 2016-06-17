// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_SHADOWS_VFX_PROGRAM_H_
#define EXAMPLES_SHADOWS_VFX_PROGRAM_H_

#include <GLES2/gl2.h>

#include "base/macros.h"

namespace vfx {
class Shader;

class Program {
 public:
  Program(Shader* vertex_shader, Shader* fragment_shader);
  ~Program();

  GLuint id() const { return id_; }

 private:
  GLuint id_;

  DISALLOW_COPY_AND_ASSIGN(Program);
};

}  // namespace vfx

#endif  // EXAMPLES_SHADOWS_VFX_PROGRAM_H_
