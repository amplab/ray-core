// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/shadows/vfx/color_program.h"

#include <memory>

#include <GLES2/gl2.h>
#include <glm/gtc/type_ptr.hpp>

#include "base/logging.h"

namespace vfx {
namespace {

const char kVertexShaderSource[] = R"GLSL(
uniform mat4 u_transform;
attribute vec3 a_position;
attribute vec4 a_color;
varying vec4 v_color;
void main() {
  gl_Position = u_transform * vec4(a_position, 1.0);
  v_color = a_color;
}
)GLSL";

const char kFragmentShaderSource[] = R"GLSL(
varying lowp vec4 v_color;
void main() {
  gl_FragColor = v_color;
}
)GLSL";

}  // namespace

ColorProgram::Vertex::Vertex() = default;

ColorProgram::Vertex::Vertex(glm::vec3 position, glm::vec4 color)
    : position(std::move(position)), color(std::move(color)) {}

ColorProgram::ColorProgram()
    : vertex_shader_(GL_VERTEX_SHADER, kVertexShaderSource),
      fragment_shader_(GL_FRAGMENT_SHADER, kFragmentShaderSource),
      program_(&vertex_shader_, &fragment_shader_),
      u_transform_(glGetUniformLocation(program_.id(), "u_transform")),
      a_position_(glGetAttribLocation(program_.id(), "a_position")),
      a_color_(glGetAttribLocation(program_.id(), "a_color")) {
  CHECK(u_transform_ != -1);
  CHECK(a_position_ != -1);
  CHECK(a_color_ != -1);
  glEnableVertexAttribArray(a_position_);
  glEnableVertexAttribArray(a_color_);
}

ColorProgram::~ColorProgram() {}

void ColorProgram::Use(const glm::mat4& transform) {
  const GLvoid* kPositionOffset = nullptr;
  const GLvoid* kColorOffset = reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3);

  glUseProgram(program_.id());
  glUniformMatrix4fv(u_transform_, 1, GL_FALSE, glm::value_ptr(transform));
  glVertexAttribPointer(a_position_, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        kPositionOffset);
  glVertexAttribPointer(a_color_, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        kColorOffset);
}

}  // namespace vfx
