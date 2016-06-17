// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This example program is based on Simple_VertexShader.c from:

//
// Book:      OpenGL(R) ES 2.0 Programming Guide
// Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
// ISBN-10:   0321502795
// ISBN-13:   9780321502797
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780321563835
//            http://www.opengles-book.com
//

#include "examples/spinning_cube/spinning_cube.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "mojo/public/c/gpu/MGL/mgl.h"
#include "mojo/public/cpp/environment/logging.h"

namespace examples {

#define VISIT_GL_CALL(Function, ReturnType, PARAMETERS, ARGUMENTS) \
  using Function##Type = ReturnType (*)PARAMETERS;
#include "mojo/public/platform/native/gles2/call_visitor_autogen.h"
#undef VISIT_GL_CALL

class GLInterface {
 public:
  GLInterface() {
#define VISIT_GL_CALL(Function, ReturnType, PARAMETERS, ARGUMENTS) \
    Function = nullptr;
#include "mojo/public/platform/native/gles2/call_visitor_autogen.h"
#undef VISIT_GL_CALL
  }

  void Init() {
#define VISIT_GL_CALL(Function, ReturnType, PARAMETERS, ARGUMENTS) \
    Function = reinterpret_cast<Function##Type>( \
        MGLGetProcAddress("gl"#Function));
#include "mojo/public/platform/native/gles2/call_visitor_autogen.h"
#undef VISIT_GL_CALL
  }

#define VISIT_GL_CALL(Function, ReturnType, PARAMETERS, ARGUMENTS) \
  Function##Type Function;
#include "mojo/public/platform/native/gles2/call_visitor_autogen.h"
#undef VISIT_GL_CALL
};

namespace {

const float kPi = 3.14159265359f;
const int kNumVertices = 24;

int GenerateCube(const scoped_ptr<GLInterface>& gl,
                 GLuint *vbo_vertices,
                 GLuint *vbo_indices) {
  const int num_indices = 36;

  const GLfloat cube_vertices[kNumVertices * 3] = {
    // -Y side.
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f, -0.5f, -0.5f,

    // +Y side.
    -0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f, -0.5f,

    // -Z side.
    -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,

    // +Z side.
    -0.5f, -0.5f, 0.5f,
    -0.5f,  0.5f, 0.5f,
    0.5f,  0.5f, 0.5f,
    0.5f, -0.5f, 0.5f,

    // -X side.
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,

    // +X side.
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f, -0.5f,
  };

  const GLfloat vertex_normals[kNumVertices * 3] = {
    // -Y side.
    0.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,

    // +Y side.
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,

    // -Z side.
    0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, -1.0f,

    // +Z side.
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,

    // -X side.
    -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,

    // +X side.
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
  };

  const GLushort cube_indices[] = {
    // -Y side.
    0, 2, 1,
    0, 3, 2,

    // +Y side.
    4, 5, 6,
    4, 6, 7,

    // -Z side.
    8, 9, 10,
    8, 10, 11,

    // +Z side.
    12, 15, 14,
    12, 14, 13,

    // -X side.
    16, 17, 18,
    16, 18, 19,

    // +X side.
    20, 23, 22,
    20, 22, 21
  };

  if (vbo_vertices) {
    gl->GenBuffers(1, vbo_vertices);
    gl->BindBuffer(GL_ARRAY_BUFFER, *vbo_vertices);
    gl->BufferData(GL_ARRAY_BUFFER,
                 sizeof(cube_vertices) + sizeof(vertex_normals),
                 nullptr,
                 GL_STATIC_DRAW);
    gl->BufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cube_vertices), cube_vertices);
    gl->BufferSubData(GL_ARRAY_BUFFER, sizeof(cube_vertices),
                    sizeof(vertex_normals), vertex_normals);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
  }

  if (vbo_indices) {
    gl->GenBuffers(1, vbo_indices);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, *vbo_indices);
    gl->BufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(cube_indices),
                 cube_indices,
                 GL_STATIC_DRAW);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  return num_indices;
}

GLuint LoadShader(const scoped_ptr<GLInterface>& gl,
                  GLenum type,
                  const char* shader_source) {
  GLuint shader = gl->CreateShader(type);
  gl->ShaderSource(shader, 1, &shader_source, NULL);
  gl->CompileShader(shader);

  GLint compiled = 0;
  gl->GetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

  if (!compiled) {
    GLsizei expected_length = 0;
    gl->GetShaderiv(shader, GL_INFO_LOG_LENGTH, &expected_length);
    std::string log;
    log.resize(expected_length);  // Includes null terminator.
    GLsizei actual_length = 0;
    gl->GetShaderInfoLog(shader, expected_length, &actual_length, &log[0]);
    log.resize(actual_length);  // Excludes null terminator.
    MOJO_LOG(FATAL) << "Compilation of shader failed: " << log;
    gl->DeleteShader(shader);
    return 0;
  }

  return shader;
}

GLuint LoadProgram(const scoped_ptr<GLInterface>& gl,
                   const char* vertex_shader_source,
                   const char* fragment_shader_source) {
  GLuint vertex_shader = LoadShader(gl,
                                    GL_VERTEX_SHADER,
                                    vertex_shader_source);
  if (!vertex_shader)
    return 0;

  GLuint fragment_shader = LoadShader(gl,
                                      GL_FRAGMENT_SHADER,
                                      fragment_shader_source);
  if (!fragment_shader) {
    gl->DeleteShader(vertex_shader);
    return 0;
  }

  GLuint program_object = gl->CreateProgram();
  gl->AttachShader(program_object, vertex_shader);
  gl->AttachShader(program_object, fragment_shader);
  gl->LinkProgram(program_object);
  gl->DeleteShader(vertex_shader);
  gl->DeleteShader(fragment_shader);

  GLint linked = 0;
  gl->GetProgramiv(program_object, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLsizei expected_length = 0;
    gl->GetProgramiv(program_object, GL_INFO_LOG_LENGTH, &expected_length);
    std::string log;
    log.resize(expected_length);  // Includes null terminator.
    GLsizei actual_length = 0;
    gl->GetProgramInfoLog(program_object, expected_length, &actual_length,
                        &log[0]);
    log.resize(actual_length);  // Excludes null terminator.
    MOJO_LOG(FATAL) << "Linking program failed: " << log;
    gl->DeleteProgram(program_object);
    return 0;
  }

  return program_object;
}

class ESMatrix {
 public:
  GLfloat m[4][4];

  ESMatrix() {
    LoadZero();
  }

  void LoadZero() {
    memset(this, 0x0, sizeof(ESMatrix));
  }

  void LoadIdentity() {
    LoadZero();
    m[0][0] = 1.0f;
    m[1][1] = 1.0f;
    m[2][2] = 1.0f;
    m[3][3] = 1.0f;
  }

  void Multiply(ESMatrix* a, ESMatrix* b) {
    ESMatrix result;
    for (int i = 0; i < 4; ++i) {
      result.m[i][0] = (a->m[i][0] * b->m[0][0]) +
                       (a->m[i][1] * b->m[1][0]) +
                       (a->m[i][2] * b->m[2][0]) +
                       (a->m[i][3] * b->m[3][0]);

      result.m[i][1] = (a->m[i][0] * b->m[0][1]) +
                       (a->m[i][1] * b->m[1][1]) +
                       (a->m[i][2] * b->m[2][1]) +
                       (a->m[i][3] * b->m[3][1]);

      result.m[i][2] = (a->m[i][0] * b->m[0][2]) +
                       (a->m[i][1] * b->m[1][2]) +
                       (a->m[i][2] * b->m[2][2]) +
                       (a->m[i][3] * b->m[3][2]);

      result.m[i][3] = (a->m[i][0] * b->m[0][3]) +
                       (a->m[i][1] * b->m[1][3]) +
                       (a->m[i][2] * b->m[2][3]) +
                       (a->m[i][3] * b->m[3][3]);
    }
    *this = result;
  }

  void Frustum(float left,
               float right,
               float bottom,
               float top,
               float near_z,
               float far_z) {
    float delta_x = right - left;
    float delta_y = top - bottom;
    float delta_z = far_z - near_z;

    if ((near_z <= 0.0f) ||
        (far_z <= 0.0f) ||
        (delta_z <= 0.0f) ||
        (delta_y <= 0.0f) ||
        (delta_y <= 0.0f))
      return;

    ESMatrix frust;
    frust.m[0][0] = 2.0f * near_z / delta_x;
    frust.m[0][1] = frust.m[0][2] = frust.m[0][3] = 0.0f;

    frust.m[1][1] = 2.0f * near_z / delta_y;
    frust.m[1][0] = frust.m[1][2] = frust.m[1][3] = 0.0f;

    frust.m[2][0] = (right + left) / delta_x;
    frust.m[2][1] = (top + bottom) / delta_y;
    frust.m[2][2] = -(near_z + far_z) / delta_z;
    frust.m[2][3] = -1.0f;

    frust.m[3][2] = -2.0f * near_z * far_z / delta_z;
    frust.m[3][0] = frust.m[3][1] = frust.m[3][3] = 0.0f;

    Multiply(&frust, this);
  }

  void Perspective(float fov_y, float aspect, float near_z, float far_z) {
    GLfloat frustum_h = tanf(fov_y / 360.0f * kPi) * near_z;
    GLfloat frustum_w = frustum_h * aspect;
    Frustum(-frustum_w, frustum_w, -frustum_h, frustum_h, near_z, far_z);
  }

  void Translate(GLfloat tx, GLfloat ty, GLfloat tz) {
    m[3][0] += m[0][0] * tx + m[1][0] * ty + m[2][0] * tz;
    m[3][1] += m[0][1] * tx + m[1][1] * ty + m[2][1] * tz;
    m[3][2] += m[0][2] * tx + m[1][2] * ty + m[2][2] * tz;
    m[3][3] += m[0][3] * tx + m[1][3] * ty + m[2][3] * tz;
  }

  void Rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    GLfloat mag = sqrtf(x * x + y * y + z * z);

    GLfloat sin_angle = sinf(angle * kPi / 180.0f);
    GLfloat cos_angle = cosf(angle * kPi / 180.0f);
    if (mag > 0.0f) {
      GLfloat xx, yy, zz, xy, yz, zx, xs, ys, zs;
      GLfloat one_minus_cos;
      ESMatrix rotation;

      x /= mag;
      y /= mag;
      z /= mag;

      xx = x * x;
      yy = y * y;
      zz = z * z;
      xy = x * y;
      yz = y * z;
      zx = z * x;
      xs = x * sin_angle;
      ys = y * sin_angle;
      zs = z * sin_angle;
      one_minus_cos = 1.0f - cos_angle;

      rotation.m[0][0] = (one_minus_cos * xx) + cos_angle;
      rotation.m[0][1] = (one_minus_cos * xy) - zs;
      rotation.m[0][2] = (one_minus_cos * zx) + ys;
      rotation.m[0][3] = 0.0F;

      rotation.m[1][0] = (one_minus_cos * xy) + zs;
      rotation.m[1][1] = (one_minus_cos * yy) + cos_angle;
      rotation.m[1][2] = (one_minus_cos * yz) - xs;
      rotation.m[1][3] = 0.0F;

      rotation.m[2][0] = (one_minus_cos * zx) - ys;
      rotation.m[2][1] = (one_minus_cos * yz) + xs;
      rotation.m[2][2] = (one_minus_cos * zz) + cos_angle;
      rotation.m[2][3] = 0.0F;

      rotation.m[3][0] = 0.0F;
      rotation.m[3][1] = 0.0F;
      rotation.m[3][2] = 0.0F;
      rotation.m[3][3] = 1.0F;

      Multiply(&rotation, this);
    }
  }
};

float RotationForTimeDelta(float delta_time) {
  return delta_time * 40.0f;
}

float RotationForDragDistance(float drag_distance) {
  return drag_distance / 5; // Arbitrary damping.
}

}  // namespace

class SpinningCube::GLState {
 public:
  GLState();

  void OnGLContextLost();

  GLfloat angle_;  // Survives losing the GL context.

  GLuint program_object_;
  GLint position_location_;
  GLint normal_location_;
  GLint color_location_;
  GLint mvp_location_;
  GLuint vbo_vertices_;
  GLuint vbo_indices_;
  int num_indices_;
  ESMatrix mvp_matrix_;
};

SpinningCube::GLState::GLState()
    : angle_(0) {
  OnGLContextLost();
}

void SpinningCube::GLState::OnGLContextLost() {
  program_object_ = 0;
  position_location_ = 0;
  normal_location_ = 0;
  color_location_ = 0;
  mvp_location_ = 0;
  vbo_vertices_ = 0;
  vbo_indices_ = 0;
  num_indices_ = 0;
}

SpinningCube::SpinningCube()
    : initialized_(false),
      width_(0),
      height_(0),
      gl_(new GLInterface()),
      state_(new GLState()),
      fling_multiplier_(1.0f),
      direction_(1),
      color_() {
  state_->angle_ = 45.0f;
  set_color(1.0, 1.0, 1.0);
}

SpinningCube::~SpinningCube() {
  if (!initialized_)
    return;
  if (state_->vbo_vertices_)
    gl_->DeleteBuffers(1, &state_->vbo_vertices_);
  if (state_->vbo_indices_)
    gl_->DeleteBuffers(1, &state_->vbo_indices_);
  if (state_->program_object_)
    gl_->DeleteProgram(state_->program_object_);
}

void SpinningCube::Init() {
  const char vertex_shader_source[] =
      "uniform mat4 u_mvpMatrix;                                       \n"
      "attribute vec4 a_position;                                      \n"
      "attribute vec4 a_normal;                                        \n"
      "uniform vec3 u_color;                                           \n"
      "varying vec4 v_color;                                           \n"
      "void main()                                                     \n"
      "{                                                               \n"
      "   gl_Position = u_mvpMatrix * a_position;                      \n"
      "   vec4 rotated_normal = u_mvpMatrix * a_normal;                \n"
      "   vec4 light_direction = normalize(vec4(0.0, 1.0, -1.0, 0.0)); \n"
      "   float directional_capture =                                  \n"
      "       clamp(dot(rotated_normal, light_direction), 0.0, 1.0);   \n"
      "   float light_intensity = 0.6 * directional_capture + 0.4;     \n"
      "   vec3 base_color = a_position.xyz + 0.5;                      \n"
      "   vec3 color = base_color * u_color;                           \n"
      "   v_color = vec4(color * light_intensity, 1.0);                \n"
      "}                                                               \n";

  const char fragment_shader_source[] =
      "precision mediump float;                            \n"
      "varying vec4 v_color;                               \n"
      "void main()                                         \n"
      "{                                                   \n"
      "   gl_FragColor = v_color;                          \n"
      "}                                                   \n";

  gl_->Init();
  state_->program_object_ = LoadProgram(
      gl_, vertex_shader_source, fragment_shader_source);
  state_->position_location_ = gl_->GetAttribLocation(
      state_->program_object_, "a_position");
  state_->normal_location_ = gl_->GetAttribLocation(
      state_->program_object_, "a_normal");
  state_->color_location_ = gl_->GetUniformLocation(
      state_->program_object_, "u_color");
  state_->mvp_location_ = gl_->GetUniformLocation(
      state_->program_object_, "u_mvpMatrix");
  state_->num_indices_ = GenerateCube(
      gl_, &state_->vbo_vertices_, &state_->vbo_indices_);

  gl_->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  gl_->Enable(GL_DEPTH_TEST);
  initialized_ = true;
}

void SpinningCube::OnGLContextLost() {
  initialized_ = false;
  state_->OnGLContextLost();
}

void SpinningCube::SetFlingMultiplier(float drag_distance,
                                      float drag_time) {
  fling_multiplier_ = RotationForDragDistance(drag_distance) /
      RotationForTimeDelta(drag_time);
}

void SpinningCube::UpdateForTimeDelta(float delta_time) {
  state_->angle_ += RotationForTimeDelta(delta_time) * fling_multiplier_;
  if (state_->angle_ >= 360.0f)
    state_->angle_ -= 360.0f;

  // Arbitrary 50-step linear reduction in spin speed.
  if (fling_multiplier_ > 1.0f) {
    fling_multiplier_ =
        std::max(1.0f, fling_multiplier_ - (fling_multiplier_ - 1.0f) / 50);
  }

  if (fling_multiplier_ < -1.0f) {
    fling_multiplier_ =
        std::min(-1.0f, fling_multiplier_ - (fling_multiplier_ + 1.0f) / 50);
  }

  Update();
}

void SpinningCube::UpdateForDragDistance(float distance) {
  state_->angle_ += RotationForDragDistance(distance);
  if (state_->angle_ >= 360.0f )
    state_->angle_ -= 360.0f;

  Update();
}

void SpinningCube::Draw() {
  gl_->Viewport(0, 0, width_, height_);
  gl_->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gl_->UseProgram(state_->program_object_);
  gl_->BindBuffer(GL_ARRAY_BUFFER, state_->vbo_vertices_);
  gl_->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, state_->vbo_indices_);
  gl_->VertexAttribPointer(state_->position_location_, 3, GL_FLOAT, GL_FALSE,
                        3 * sizeof(GLfloat), 0);
  gl_->VertexAttribPointer(state_->normal_location_, 3, GL_FLOAT, GL_FALSE,
                        3 * sizeof(GLfloat),
                        reinterpret_cast<void*>(3 * sizeof(GLfloat) *
                                                kNumVertices));
  gl_->EnableVertexAttribArray(state_->position_location_);
  gl_->EnableVertexAttribArray(state_->normal_location_);
  gl_->UniformMatrix4fv(state_->mvp_location_, 1, GL_FALSE,
                     static_cast<GLfloat*>(&state_->mvp_matrix_.m[0][0]));
  gl_->Uniform3fv(state_->color_location_, 1, color_);
  gl_->DrawElements(GL_TRIANGLES, state_->num_indices_, GL_UNSIGNED_SHORT, 0);
}

void SpinningCube::Update() {
  float aspect = static_cast<GLfloat>(width_) / static_cast<GLfloat>(height_);

  ESMatrix perspective;
  perspective.LoadIdentity();
  perspective.Perspective(60.0f, aspect, 1.0f, 20.0f );

  ESMatrix modelview;
  modelview.LoadIdentity();
  modelview.Translate(0.0, 0.0, -2.0);
  modelview.Rotate(state_->angle_ * direction_, 1.0, 0.0, 1.0);

  state_->mvp_matrix_.Multiply(&modelview, &perspective);
}

}  // namespace examples
