// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/ui/ganesh_renderer.h"

#include "base/bind.h"
#include "base/logging.h"
#include "mojo/gpu/gl_texture.h"
#include "mojo/skia/ganesh_texture_surface.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace mojo {
namespace ui {

GaneshRenderer::GaneshRenderer(
    const scoped_refptr<mojo::skia::GaneshContext>& ganesh_context)
    : ganesh_context_(ganesh_context),
      gl_renderer_(ganesh_context_->gl_context()) {
  DCHECK(ganesh_context_);
}

GaneshRenderer::~GaneshRenderer() {}

mojo::gfx::composition::ResourcePtr GaneshRenderer::DrawSurface(
    const mojo::skia::GaneshContext::Scope& ganesh_scope,
    const mojo::Size& size,
    const DrawSurfaceCallback& callback) {
  DCHECK(ganesh_scope.ganesh_context() == ganesh_context_);

  std::unique_ptr<mojo::GLTexture> texture =
      gl_renderer_.GetTexture(ganesh_scope.gl_scope(), size);
  DCHECK(texture);

  mojo::skia::GaneshTextureSurface texture_surface(ganesh_scope,
                                                   std::move(texture));
  callback.Run(ganesh_scope, size, texture_surface.surface());
  texture = texture_surface.TakeTexture();

  return gl_renderer_.BindTextureResource(
      ganesh_scope.gl_scope(), std::move(texture),
      mojo::gfx::composition::MailboxTextureResource::Origin::TOP_LEFT);
}

mojo::gfx::composition::ResourcePtr GaneshRenderer::DrawSurface(
    const mojo::Size& size,
    const DrawSurfaceCallback& callback) {
  if (ganesh_context_->is_lost())
    return nullptr;
  mojo::skia::GaneshContext::Scope ganesh_scope(ganesh_context_);
  return DrawSurface(ganesh_scope, size, callback);
}

static void RunCanvasCallback(
    const mojo::ui::GaneshRenderer::DrawCanvasCallback& callback,
    const mojo::skia::GaneshContext::Scope& ganesh_scope,
    const mojo::Size& size,
    SkSurface* surface) {
  callback.Run(ganesh_scope, size, surface->getCanvas());
}

mojo::gfx::composition::ResourcePtr GaneshRenderer::DrawCanvas(
    const mojo::skia::GaneshContext::Scope& ganesh_scope,
    const mojo::Size& size,
    const DrawCanvasCallback& callback) {
  return DrawSurface(ganesh_scope, size,
                     base::Bind(&RunCanvasCallback, callback));
}

mojo::gfx::composition::ResourcePtr GaneshRenderer::DrawCanvas(
    const mojo::Size& size,
    const DrawCanvasCallback& callback) {
  if (ganesh_context_->is_lost())
    return nullptr;
  mojo::skia::GaneshContext::Scope ganesh_scope(ganesh_context_);
  return DrawCanvas(ganesh_scope, size, callback);
}

}  // namespace ui
}  // namespace mojo
