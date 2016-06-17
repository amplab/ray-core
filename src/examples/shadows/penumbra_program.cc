// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/shadows/penumbra_program.h"

#include <memory>
#include <GLES2/gl2.h>
#include <glm/gtc/type_ptr.hpp>

#include "base/logging.h"

namespace examples {
namespace {

const char kVertexShaderSource[] = R"GLSL(
uniform mat4 u_transform;

attribute vec3 a_position;
attribute vec3 a_occluder0;
attribute vec3 a_occluder1;
attribute vec3 a_occluder2;
attribute vec3 a_occluder3;

varying vec3 v_occluder0;
varying vec3 v_occluder1;
varying vec3 v_occluder2;
varying vec3 v_occluder3;

void main() {
  gl_Position = u_transform * vec4(a_position, 1.0);

  vec4 h_occluder0 = u_transform * vec4(a_occluder0, 1.0);
  vec4 h_occluder1 = u_transform * vec4(a_occluder1, 1.0);
  vec4 h_occluder2 = u_transform * vec4(a_occluder2, 1.0);
  vec4 h_occluder3 = u_transform * vec4(a_occluder3, 1.0);

  v_occluder0 = h_occluder0.xyz / h_occluder0.w;
  v_occluder1 = h_occluder1.xyz / h_occluder1.w;
  v_occluder2 = h_occluder2.xyz / h_occluder2.w;
  v_occluder3 = h_occluder3.xyz / h_occluder3.w;
}
)GLSL";

const char kFragmentShaderSource[] = R"GLSL(
precision highp float;

varying highp vec3 v_occluder0;
varying highp vec3 v_occluder1;
varying highp vec3 v_occluder2;
varying highp vec3 v_occluder3;
uniform vec2 u_inverse_size;

const highp vec4 zero4 = vec4(0.0, 0.0, 0.0, 0.0);

// TODO(abarth): Currently the light is located and sized in normalized device
// coordinates, which are viewport-relative. We need to instead specify the
// light in the world coordinate system and transform it by u_transform into
// normalized device coordinates.
const highp float light_z = 0.2;
const highp float light_min_x = 0.0;
const highp float light_min_y = 0.0;
const highp float light_max_x = 1.0;
const highp float light_max_y = 1.0;

highp float getMin(vec4 value) {
  return min(min(value.x, value.y), min(value.z, value.w));
}

highp float getMax(vec4 value) {
  return max(max(value.x, value.y), max(value.z, value.w));
}

void main(void) {
  // Convert gl_FragCoord from window coordiates to [0:1]^2 for texture lookup.
  highp vec2 depth_coord = gl_FragCoord.xy * u_inverse_size;

  // Depth is in the range [0:1].
  // TODO(abarth): Fetch the depth from a depth texture.
  highp float depth = 0.3;

  // Convert from [0:1]^3 to [-1:1]^3 (i.e., normalized device coordinates).
  highp vec3 position = vec3(depth_coord, depth) * 2.0 - 1.0;

  highp float light_distance = light_z - position.z;

  highp vec3 ray0 = v_occluder0 - position;
  highp vec3 ray1 = v_occluder1 - position;
  highp vec3 ray2 = v_occluder2 - position;
  highp vec3 ray3 = v_occluder3 - position;

  highp float scale0 = light_distance / ray0.z;
  highp float scale1 = light_distance / ray1.z;
  highp float scale2 = light_distance / ray2.z;
  highp float scale3 = light_distance / ray3.z;

  highp vec4 horizontal = clamp(vec4(scale0 * ray0.x,
                                     scale1 * ray1.x,
                                     scale2 * ray2.x,
                                     scale3 * ray3.x),
                                light_min_x, light_max_x);
  highp vec4 vertical   = clamp(vec4(scale0 * ray0.y,
                                     scale1 * ray1.y,
                                     scale2 * ray2.y,
                                     scale3 * ray3.y),
                                light_min_y, light_max_y);

  lowp float occulded = (getMax(horizontal) - getMin(horizontal)) *
                        (getMax(vertical)   - getMin(vertical));

  lowp float illuminated = 1.0 - occulded;

  gl_FragColor = vec4(illuminated, illuminated, illuminated, 1.0);
}
)GLSL";

GLvoid* GetOffset(int n) {
  return reinterpret_cast<GLvoid*>(sizeof(GLfloat) * n);
}

GLint AddVertexAttribPointer(GLint index, GLint attribute, GLint count) {
  glVertexAttribPointer(attribute, count, GL_FLOAT, GL_FALSE,
                        sizeof(PenumbraProgram::Vertex), GetOffset(index));
  return index + count;
}

}  // namespace

PenumbraProgram::Vertex::Vertex() = default;

PenumbraProgram::Vertex::Vertex(glm::vec3 point,
                                glm::vec3 occluder0,
                                glm::vec3 occluder1,
                                glm::vec3 occluder2,
                                glm::vec3 occluder3)
    : point(std::move(point)),
      occluder0(std::move(occluder0)),
      occluder1(std::move(occluder1)),
      occluder2(std::move(occluder2)),
      occluder3(std::move(occluder3)) {
}

PenumbraProgram::PenumbraProgram()
    : vertex_shader_(GL_VERTEX_SHADER, kVertexShaderSource),
      fragment_shader_(GL_FRAGMENT_SHADER, kFragmentShaderSource),
      program_(&vertex_shader_, &fragment_shader_),
      u_transform_(glGetUniformLocation(program_.id(), "u_transform")),
      u_inverse_size_(glGetUniformLocation(program_.id(), "u_inverse_size")),
      a_position_(glGetAttribLocation(program_.id(), "a_position")),
      a_occluder0_(glGetAttribLocation(program_.id(), "a_occluder0")),
      a_occluder1_(glGetAttribLocation(program_.id(), "a_occluder1")),
      a_occluder2_(glGetAttribLocation(program_.id(), "a_occluder2")),
      a_occluder3_(glGetAttribLocation(program_.id(), "a_occluder3")) {
  CHECK(u_transform_ != -1);
  CHECK(u_inverse_size_ != -1);
  CHECK(a_position_ != -1);
  CHECK(a_occluder0_ != -1);
  CHECK(a_occluder1_ != -1);
  CHECK(a_occluder2_ != -1);
  CHECK(a_occluder3_ != -1);
  glEnableVertexAttribArray(a_position_);
  glEnableVertexAttribArray(a_occluder0_);
  glEnableVertexAttribArray(a_occluder1_);
  glEnableVertexAttribArray(a_occluder2_);
  glEnableVertexAttribArray(a_occluder3_);
}

PenumbraProgram::~PenumbraProgram() {
}

void PenumbraProgram::Use(const glm::mat4& transform,
                          const glm::vec2& inverse_size) {
  glUseProgram(program_.id());
  glUniformMatrix4fv(u_transform_, 1, GL_FALSE, glm::value_ptr(transform));
  glUniform2f(u_inverse_size_, inverse_size.x, inverse_size.y);
  GLint i = 0;
  i = AddVertexAttribPointer(i, a_position_, 3);
  i = AddVertexAttribPointer(i, a_occluder0_, 3);
  i = AddVertexAttribPointer(i, a_occluder1_, 3);
  i = AddVertexAttribPointer(i, a_occluder2_, 3);
  AddVertexAttribPointer(i, a_occluder3_, 3);
}

}  // namespace examples
