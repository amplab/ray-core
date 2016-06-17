// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_SHADOWS_PENUMBRA_PROGRAM_H_
#define EXAMPLES_SHADOWS_PENUMBRA_PROGRAM_H_

#include <glm/glm.hpp>

#include "base/macros.h"
#include "examples/shadows/vfx/program.h"
#include "examples/shadows/vfx/shader.h"

namespace examples {

class PenumbraProgram {
 public:
  PenumbraProgram();
  ~PenumbraProgram();

  struct Vertex {
    Vertex();
    Vertex(glm::vec3 point,
           glm::vec3 occluder0,
           glm::vec3 occluder1,
           glm::vec3 occluder2,
           glm::vec3 occluder3);

    glm::vec3 point;
    glm::vec3 occluder0;
    glm::vec3 occluder1;
    glm::vec3 occluder2;
    glm::vec3 occluder3;
  };

  GLuint id() const { return program_.id(); }

  void Use(const glm::mat4& transform, const glm::vec2& inverse_size);

 private:
  vfx::Shader vertex_shader_;
  vfx::Shader fragment_shader_;
  vfx::Program program_;

  GLint u_transform_;
  GLint u_inverse_size_;
  GLint a_position_;
  GLint a_occluder0_;
  GLint a_occluder1_;
  GLint a_occluder2_;
  GLint a_occluder3_;

  DISALLOW_COPY_AND_ASSIGN(PenumbraProgram);
};

}  // namespace examples

#endif  // EXAMPLES_SHADOWS_PENUMBRA_PROGRAM_H_
