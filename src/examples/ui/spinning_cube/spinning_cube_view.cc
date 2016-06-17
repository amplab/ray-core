// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ui/spinning_cube/spinning_cube_view.h"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2extmojo.h>

#include <cmath>

#include "base/bind.h"
#include "mojo/public/cpp/system/time.h"
#include "mojo/services/geometry/cpp/geometry_util.h"

namespace examples {

namespace {
constexpr uint32_t kCubeImageResourceId = 1;
constexpr uint32_t kRootNodeId = mojo::gfx::composition::kSceneRootNodeId;

// TODO(johngro) : investigate extending mojom with a formal flags type which it
// generates good bindings for, so we don't need to resort to this.
constexpr bool operator&(const mojo::EventFlags& f1,
                         const mojo::EventFlags& f2) {
  return ((static_cast<uint32_t>(f1) & static_cast<uint32_t>(f2)) != 0);
}

float CalculateDragDistance(const mojo::PointF& start,
                            const mojo::PointF& end) {
  return std::hypot(start.x - end.x, start.y - end.y);
}

float GetRandomColor() {
  return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

// Return a direction multiplier to apply to drag distances:
// 1 for natural (positive) motion, -1 for reverse (negative) motion
int GetEventDirection(const mojo::PointF& current,
                      const mojo::PointF& initial,
                      const mojo::PointF& last) {
  // Axis of motion is determined by coarse alignment of overall movement
  bool use_x =
      std::abs(current.y - initial.y) < std::abs(current.x - initial.x);
  // Current direction is determined by comparison with previous point
  float delta = use_x ? (current.x - last.x) : (current.y - last.y);
  return delta > 0 ? -1 : 1;
}
}  // namespace

SpinningCubeView::SpinningCubeView(
    mojo::InterfaceHandle<mojo::ApplicationConnector> app_connector,
    mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request)
    : GLView(app_connector.Pass(), view_owner_request.Pass(), "SpinningCube"),
      input_handler_(GetViewServiceProvider(), this),
      weak_ptr_factory_(this) {
  mojo::GLContext::Scope gl_scope(gl_context());
  cube_.Init();
}

SpinningCubeView::~SpinningCubeView() {}

void SpinningCubeView::OnEvent(mojo::EventPtr event,
                               const OnEventCallback& callback) {
  if (!event->pointer_data) {
    callback.Run(false);
    return;
  }

  switch (event->action) {
    case mojo::EventType::POINTER_DOWN:
      if (event->flags & mojo::EventFlags::RIGHT_MOUSE_BUTTON)
        break;
      capture_point_.x = event->pointer_data->x;
      capture_point_.y = event->pointer_data->y;
      last_drag_point_ = capture_point_;
      drag_start_time_ = mojo::GetTimeTicksNow();
      cube_.SetFlingMultiplier(0.0f, 1.0f);
      break;

    case mojo::EventType::POINTER_MOVE: {
      if (!(event->flags & mojo::EventFlags::LEFT_MOUSE_BUTTON) &&
          event->pointer_data->kind == mojo::PointerKind::MOUSE) {
        break;
      }
      mojo::PointF event_location;
      event_location.x = event->pointer_data->x;
      event_location.y = event->pointer_data->y;
      int direction =
          GetEventDirection(event_location, capture_point_, last_drag_point_);
      cube_.UpdateForDragDistance(
          direction * CalculateDragDistance(last_drag_point_, event_location));
      last_drag_point_ = event_location;
      break;
    }

    case mojo::EventType::POINTER_UP: {
      if (event->flags & mojo::EventFlags::RIGHT_MOUSE_BUTTON) {
        cube_.set_color(GetRandomColor(), GetRandomColor(), GetRandomColor());
        break;
      }
      mojo::PointF event_location;
      event_location.x = event->pointer_data->x;
      event_location.y = event->pointer_data->y;
      MojoTimeTicks offset = mojo::GetTimeTicksNow() - drag_start_time_;
      float delta = static_cast<float>(offset) / 1000000.f;
      // Last drag point is the same as current point here; use initial capture
      // point instead
      int direction =
          GetEventDirection(event_location, capture_point_, capture_point_);
      cube_.SetFlingMultiplier(
          direction * CalculateDragDistance(capture_point_, event_location),
          delta);
      capture_point_ = last_drag_point_ = mojo::PointF();
      break;
    }

    default:
      break;
  }

  callback.Run(true);
}

void SpinningCubeView::OnDraw() {
  DCHECK(properties());

  // Update the state of the cube.
  cube_.UpdateForTimeDelta(frame_tracker().frame_time_delta() * 0.000001f);

  // Update the contents of the scene.
  auto update = mojo::gfx::composition::SceneUpdate::New();

  const mojo::Size& size = *properties()->view_layout->size;
  if (size.width > 0 && size.height > 0) {
    mojo::RectF bounds;
    bounds.width = size.width;
    bounds.height = size.height;

    mojo::gfx::composition::ResourcePtr cube_resource = gl_renderer()->DrawGL(
        size, true,
        base::Bind(&SpinningCubeView::DrawCubeWithGL, base::Unretained(this)));
    DCHECK(cube_resource);
    update->resources.insert(kCubeImageResourceId, cube_resource.Pass());

    auto root_node = mojo::gfx::composition::Node::New();
    root_node->hit_test_behavior =
        mojo::gfx::composition::HitTestBehavior::New();
    root_node->op = mojo::gfx::composition::NodeOp::New();
    root_node->op->set_image(mojo::gfx::composition::ImageNodeOp::New());
    root_node->op->get_image()->content_rect = bounds.Clone();
    root_node->op->get_image()->image_resource_id = kCubeImageResourceId;
    update->nodes.insert(kRootNodeId, root_node.Pass());
  } else {
    auto root_node = mojo::gfx::composition::Node::New();
    update->nodes.insert(kRootNodeId, root_node.Pass());
  }

  scene()->Update(update.Pass());

  // Publish the scene.
  scene()->Publish(CreateSceneMetadata());

  // Loop!
  Invalidate();
}

void SpinningCubeView::DrawCubeWithGL(const mojo::GLContext::Scope& gl_scope,
                                      const mojo::Size& size) {
  cube_.set_size(size.width, size.height);
  cube_.Draw();
}

}  // namespace examples
