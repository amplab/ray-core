// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include "examples/spinning_cube/gles2_client_impl.h"

#include <math.h>
#include <stdlib.h>
#include <cmath>
#include <utility>

#include "mojo/public/c/gpu/MGL/mgl.h"
#include "mojo/public/c/gpu/MGL/mgl_onscreen.h"
#include "mojo/public/cpp/environment/environment.h"
#include "mojo/public/cpp/system/time.h"
#include "mojo/public/cpp/utility/run_loop.h"

namespace examples {
namespace {

// TODO(johngro) : investigate extending mojom with a formal flags type which it
// generates good bindings for, so we don't need to resort to this.
static inline constexpr bool operator &(const mojo::EventFlags& f1,
                                        const mojo::EventFlags& f2) {
  return ((static_cast<uint32_t>(f1) & static_cast<uint32_t>(f2)) != 0);
}

float CalculateDragDistance(const mojo::PointF& start,
                            const mojo::PointF& end) {
  return hypot(start.x - end.x, start.y - end.y);
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
  bool use_x = std::abs(current.y - initial.y) <
               std::abs(current.x - initial.x);
  // Current direction is determined by comparison with previous point
  float delta = use_x ? (current.x - last.x)
                      : (current.y - last.y);
  return delta > 0 ? -1 : 1;
}
}  // namespace

GLES2ClientImpl::GLES2ClientImpl(mojo::ContextProviderPtr context_provider)
    : last_time_(mojo::GetTimeTicksNow()),
      waiting_to_draw_(false),
      context_provider_(context_provider.Pass()),
      context_(nullptr) {
  context_provider_->Create(
      nullptr,
      [this](mojo::InterfaceHandle<mojo::CommandBuffer> command_buffer) {
        ContextCreated(std::move(command_buffer));
      });
}

GLES2ClientImpl::~GLES2ClientImpl() {
  MGLDestroyContext(context_);
}

void GLES2ClientImpl::SetSize(const mojo::Size& size) {
  size_ = size;
  cube_.set_size(size_.width, size_.height);
  if (size_.width == 0 || size_.height == 0 || !context_)
    return;
  MGLResizeSurface(size_.width, size_.height);
  WantToDraw();
}

void GLES2ClientImpl::HandleInputEvent(const mojo::Event& event) {
  switch (event.action) {
    case mojo::EventType::POINTER_DOWN:
      if (event.flags & mojo::EventFlags::RIGHT_MOUSE_BUTTON)
        break;
      capture_point_.x = event.pointer_data->x;
      capture_point_.y = event.pointer_data->y;
      last_drag_point_ = capture_point_;
      drag_start_time_ = mojo::GetTimeTicksNow();
      cube_.SetFlingMultiplier(0.0f, 1.0f);
      break;
    case mojo::EventType::POINTER_MOVE: {
      if (!(event.flags & mojo::EventFlags::LEFT_MOUSE_BUTTON) &&
          event.pointer_data->kind == mojo::PointerKind::MOUSE) {
        break;
      }
      mojo::PointF event_location;
      event_location.x = event.pointer_data->x;
      event_location.y = event.pointer_data->y;
      int direction = GetEventDirection(event_location,
                                        capture_point_,
                                        last_drag_point_);
      cube_.UpdateForDragDistance(
          direction * CalculateDragDistance(last_drag_point_, event_location));
      WantToDraw();

      last_drag_point_ = event_location;
      break;
    }
    case mojo::EventType::POINTER_UP: {
      if (event.flags & mojo::EventFlags::RIGHT_MOUSE_BUTTON) {
        cube_.set_color(GetRandomColor(), GetRandomColor(), GetRandomColor());
        break;
      }
      mojo::PointF event_location;
      event_location.x = event.pointer_data->x;
      event_location.y = event.pointer_data->y;
      MojoTimeTicks offset = mojo::GetTimeTicksNow() - drag_start_time_;
      float delta = static_cast<float>(offset) / 1000000.f;
      // Last drag point is the same as current point here; use initial capture
      // point instead
      int direction = GetEventDirection(event_location,
                                        capture_point_,
                                        capture_point_);
      cube_.SetFlingMultiplier(
          direction * CalculateDragDistance(capture_point_, event_location),
          delta);

      capture_point_ = last_drag_point_ = mojo::PointF();
      WantToDraw();
      break;
    }
    default:
      break;
  }
}

void GLES2ClientImpl::ContextCreated(
    mojo::InterfaceHandle<mojo::CommandBuffer> command_buffer) {
  context_ = MGLCreateContext(MGL_API_VERSION_GLES2,
                              command_buffer.PassHandle().release().value(),
                              MGL_NO_CONTEXT, &ContextLostThunk, this,
                              mojo::Environment::GetDefaultAsyncWaiter());
  MGLMakeCurrent(context_);
  cube_.Init();
  WantToDraw();
}

void GLES2ClientImpl::ContextLost() {
  cube_.OnGLContextLost();
  MGLDestroyContext(context_);
  context_ = nullptr;
  context_provider_->Create(
      nullptr,
      [this](mojo::InterfaceHandle<mojo::CommandBuffer> command_buffer) {
        ContextCreated(std::move(command_buffer));
      });
}

void GLES2ClientImpl::ContextLostThunk(void* closure) {
  static_cast<GLES2ClientImpl*>(closure)->ContextLost();
}

void GLES2ClientImpl::WantToDraw() {
  if (waiting_to_draw_ || !context_)
    return;
  waiting_to_draw_ = true;
  mojo::RunLoop::current()->PostDelayedTask([this]() { Draw(); },
                                            MojoTimeTicks(16667));
}

void GLES2ClientImpl::Draw() {
  waiting_to_draw_ = false;
  if (!context_)
    return;
  MojoTimeTicks now = mojo::GetTimeTicksNow();
  MojoTimeTicks offset = now - last_time_;
  float delta = static_cast<float>(offset) / 1000000.;
  last_time_ = now;
  cube_.UpdateForTimeDelta(delta);
  cube_.Draw();

  MGLSwapBuffers();
  WantToDraw();
}

}  // namespace examples
