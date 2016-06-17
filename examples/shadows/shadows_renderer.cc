// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/shadows/shadows_view.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

namespace examples {

ShadowsRenderer::ShadowsRenderer() {
  vec4 green(0.f, 1.f, 0.f, 1.f);
  shapes_.AddQuad({vec3( 2.f, -2.f, -2.f), green},
                  {vec3( 2.f,  2.f, -2.f), green},
                  {vec3(-2.f,  2.f, -2.f), green},
                  {vec3(-2.f, -2.f, -2.f), green});
  vec4 white(1.f, 1.f, 1.f, 1.f);
  shapes_.AddQuad({vec3( 20.f, -20.f, -5.f), white},
                  {vec3( 20.f,  20.f, -5.f), white},
                  {vec3(-20.f,  20.f, -5.f), white},
                  {vec3(-20.f, -20.f, -5.f), white});
  shapes_.BufferData(GL_STATIC_DRAW);

  vec3 occluder0( 2.f, -2.f, -2.f);
  vec3 occluder1( 2.f,  2.f, -2.f);
  vec3 occluder2(-2.f,  2.f, -2.f);
  vec3 occluder3(-2.f, -2.f, -2.f);
  penumbra_.AddQuad({vec3( 20.f, -20.f, -5.f), occluder0, occluder1,
                                               occluder2, occluder3},
                    {vec3( 20.f,  20.f, -5.f), occluder0, occluder1,
                                               occluder2, occluder3},
                    {vec3(-20.f,  20.f, -5.f), occluder0, occluder1,
                                               occluder2, occluder3},
                    {vec3(-20.f, -20.f, -5.f), occluder0, occluder1,
                                               occluder2, occluder3});
  penumbra_.BufferData(GL_STATIC_DRAW);
}

ShadowsRenderer::~ShadowsRenderer() {}

void ShadowsRenderer::Render(const mojo::Size& size) {
  glViewport(0, 0, size.width, size.height);
  glClearColor(0.f, 0.f, 1.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  float aspect =
      static_cast<GLfloat>(size.width) / static_cast<GLfloat>(size.height);
  mat4 mvp = glm::perspective(60.f, aspect, 1.f, 20.f);
  vec2 inverse_size = vec2(1.f / size.width, 1.f / size.height);

  penumbra_.Bind();
  penumbra_program_.Use(mvp, inverse_size);
  penumbra_.Draw();
}

}  // namespace examples
