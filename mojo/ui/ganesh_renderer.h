// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_UI_GANESH_RENDERER_H_
#define MOJO_UI_GANESH_RENDERER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "mojo/services/gfx/composition/interfaces/resources.mojom.h"
#include "mojo/skia/ganesh_context.h"
#include "mojo/ui/gl_renderer.h"

class SkCanvas;
class SkSurface;

namespace mojo {
namespace ui {

// Provides support for rendering Skia commands into surfaces backed by
// a pool of textures and producing scene resources for them.
class GaneshRenderer {
 public:
  // Called with an active GaneshContext and a Skia surface or canvas
  // to draw into the framebuffer.
  using DrawSurfaceCallback =
      base::Callback<void(const mojo::skia::GaneshContext::Scope& ganesh_scope,
                          const mojo::Size& size,
                          SkSurface* surface)>;
  using DrawCanvasCallback =
      base::Callback<void(const mojo::skia::GaneshContext::Scope& ganesh_scope,
                          const mojo::Size& size,
                          SkCanvas* canvas)>;

  // Creates a Ganesh backed renderer.
  explicit GaneshRenderer(
      const scoped_refptr<mojo::skia::GaneshContext>& ganesh_context);
  ~GaneshRenderer();

  // Gets the GL context, never null.
  const scoped_refptr<mojo::GLContext>& gl_context() const {
    return ganesh_context_->gl_context();
  }

  // Gets the Ganesh context, never null.
  const scoped_refptr<mojo::skia::GaneshContext>& ganesh_context() const {
    return ganesh_context_;
  }

  // Allocates a GL texture, binds it to a Ganesh surface or canvas,
  // invokes the provided function, then returns the resulting resource.
  // Never returns null.
  mojo::gfx::composition::ResourcePtr DrawSurface(
      const mojo::skia::GaneshContext::Scope& ganesh_scope,
      const mojo::Size& size,
      const DrawSurfaceCallback& callback);
  mojo::gfx::composition::ResourcePtr DrawCanvas(
      const mojo::skia::GaneshContext::Scope& ganesh_scope,
      const mojo::Size& size,
      const DrawCanvasCallback& callback);

  // Like the above but enters the Ganesh scope automatically.
  // Returns null if the GL context was lost.
  mojo::gfx::composition::ResourcePtr DrawSurface(
      const mojo::Size& size,
      const DrawSurfaceCallback& callback);
  mojo::gfx::composition::ResourcePtr DrawCanvas(
      const mojo::Size& size,
      const DrawCanvasCallback& callback);

 private:
  scoped_refptr<mojo::skia::GaneshContext> ganesh_context_;
  mojo::ui::GLRenderer gl_renderer_;

  DISALLOW_COPY_AND_ASSIGN(GaneshRenderer);
};

}  // namespace ui
}  // namespace mojo

#endif  // MOJO_UI_GANESH_RENDERER_H_
