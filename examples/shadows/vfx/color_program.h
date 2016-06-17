// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_SHADOWS_VFX_COLOR_PROGRAM_H_
#define EXAMPLES_SHADOWS_VFX_COLOR_PROGRAM_H_

#include <glm/glm.hpp>

#include "base/macros.h"
#include "examples/shadows/vfx/program.h"
#include "examples/shadows/vfx/shader.h"

namespace vfx {

class ColorProgram {
 public:
  ColorProgram();
  ~ColorProgram();

  struct Vertex {
    Vertex();
    Vertex(glm::vec3 position, glm::vec4 color);

    glm::vec3 position;
    glm::vec4 color;
  };

  GLuint id() const { return program_.id(); }

  void Use(const glm::mat4& transform);

 private:
  Shader vertex_shader_;
  Shader fragment_shader_;
  Program program_;

  GLint u_transform_;
  GLint a_position_;
  GLint a_color_;

  DISALLOW_COPY_AND_ASSIGN(ColorProgram);
};

}  // namespace vfx

#endif  // EXAMPLES_SHADOWS_VFX_COLOR_PROGRAM_H_
