// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/shadows/shadows_view.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "base/bind.h"
#include "base/logging.h"
#include "mojo/services/geometry/cpp/geometry_util.h"

namespace examples {

namespace {
constexpr uint32_t kContentImageResourceId = 1;
constexpr uint32_t kRootNodeId = mojo::gfx::composition::kSceneRootNodeId;
}  // namespace

ShadowsView::ShadowsView(
    mojo::InterfaceHandle<mojo::ApplicationConnector> app_connector,
    mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request)
    : GLView(app_connector.Pass(), view_owner_request.Pass(), "Shadows") {
  mojo::GLContext::Scope gl_scope(gl_context());
  renderer_.reset(new ShadowsRenderer());
}

ShadowsView::~ShadowsView() {}

void ShadowsView::OnDraw() {
  DCHECK(properties());

  // Update the contents of the scene.
  auto update = mojo::gfx::composition::SceneUpdate::New();

  const mojo::Size& size = *properties()->view_layout->size;
  if (size.width > 0 && size.height > 0) {
    mojo::RectF bounds;
    bounds.width = size.width;
    bounds.height = size.height;

    mojo::gfx::composition::ResourcePtr content_resource =
        gl_renderer()->DrawGL(size, true, base::Bind(&ShadowsView::Render,
                                                     base::Unretained(this)));
    DCHECK(content_resource);
    update->resources.insert(kContentImageResourceId, content_resource.Pass());

    auto root_node = mojo::gfx::composition::Node::New();
    root_node->hit_test_behavior =
        mojo::gfx::composition::HitTestBehavior::New();
    root_node->op = mojo::gfx::composition::NodeOp::New();
    root_node->op->set_image(mojo::gfx::composition::ImageNodeOp::New());
    root_node->op->get_image()->content_rect = bounds.Clone();
    root_node->op->get_image()->image_resource_id = kContentImageResourceId;
    update->nodes.insert(kRootNodeId, root_node.Pass());
  } else {
    auto root_node = mojo::gfx::composition::Node::New();
    update->nodes.insert(kRootNodeId, root_node.Pass());
  }

  scene()->Update(update.Pass());

  // Publish the scene.
  scene()->Publish(CreateSceneMetadata());
}

void ShadowsView::Render(const mojo::GLContext::Scope& gl_scope,
                         const mojo::Size& size) {
  renderer_->Render(size);
}

}  // namespace examples
