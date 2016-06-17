// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ui/noodles/rasterizer.h"

#include "base/bind.h"
#include "base/logging.h"
#include "examples/ui/noodles/frame.h"
#include "mojo/gpu/gl_context.h"
#include "mojo/skia/ganesh_context.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace examples {

constexpr uint32_t kContentImageResourceId = 1;
constexpr uint32_t kRootNodeId = mojo::gfx::composition::kSceneRootNodeId;

Rasterizer::Rasterizer(mojo::ApplicationConnectorPtr connector,
                       mojo::gfx::composition::ScenePtr scene)
    : ganesh_renderer_(new mojo::skia::GaneshContext(
          mojo::GLContext::CreateOffscreen(connector.get()))),
      scene_(scene.Pass()) {}

Rasterizer::~Rasterizer() {}

static void DrawFrame(Frame* frame,
                      const mojo::skia::GaneshContext::Scope& ganesh_scope,
                      const mojo::Size& size,
                      SkCanvas* canvas) {
  frame->Paint(canvas);
}

void Rasterizer::PublishFrame(std::unique_ptr<Frame> frame) {
  DCHECK(frame);

  auto update = mojo::gfx::composition::SceneUpdate::New();

  if (frame->size().width > 0 && frame->size().height > 0) {
    mojo::RectF bounds;
    bounds.width = frame->size().width;
    bounds.height = frame->size().height;

    mojo::gfx::composition::ResourcePtr content_resource =
        ganesh_renderer_.DrawCanvas(
            frame->size(),
            base::Bind(&DrawFrame, base::Unretained(frame.get())));
    DCHECK(content_resource);
    update->resources.insert(kContentImageResourceId, content_resource.Pass());

    auto root_node = mojo::gfx::composition::Node::New();
    root_node->op = mojo::gfx::composition::NodeOp::New();
    root_node->op->set_image(mojo::gfx::composition::ImageNodeOp::New());
    root_node->op->get_image()->content_rect = bounds.Clone();
    root_node->op->get_image()->image_resource_id = kContentImageResourceId;
    update->nodes.insert(kRootNodeId, root_node.Pass());
  } else {
    auto root_node = mojo::gfx::composition::Node::New();
    update->nodes.insert(kRootNodeId, root_node.Pass());
  }

  scene_->Update(update.Pass());
  scene_->Publish(frame->TakeSceneMetadata());
}

}  // namespace examples
