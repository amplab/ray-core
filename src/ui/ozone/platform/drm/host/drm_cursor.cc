// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/host/drm_cursor.h"

#include "base/thread_task_runner_handle.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/ozone/platform/drm/host/drm_window_host.h"
#include "ui/ozone/platform/drm/host/drm_window_host_manager.h"

#if defined(OS_CHROMEOS)
#include "ui/events/ozone/chromeos/cursor_controller.h"
#endif

namespace ui {

DrmCursor::DrmCursor(DrmWindowHostManager* window_manager) {
}

DrmCursor::~DrmCursor() {
}

void DrmCursor::SetCursor(gfx::AcceleratedWidget window,
                          PlatformCursor platform_cursor) {
  //TODO
}

void DrmCursor::OnWindowAdded(gfx::AcceleratedWidget window,
                              const gfx::Rect& bounds_in_screen,
                              const gfx::Rect& cursor_confined_bounds) {
  //TODO
}

void DrmCursor::OnWindowRemoved(gfx::AcceleratedWidget window) {
  //TODO
}

void DrmCursor::CommitBoundsChange(
    gfx::AcceleratedWidget window,
    const gfx::Rect& new_display_bounds_in_screen,
    const gfx::Rect& new_confined_bounds) {
  //TODO
}

void DrmCursor::MoveCursorTo(gfx::AcceleratedWidget window,
                             const gfx::PointF& location) {
  //TODO
}

void DrmCursor::MoveCursorTo(const gfx::PointF& screen_location) {
  //TODO
}

void DrmCursor::MoveCursor(const gfx::Vector2dF& delta) {
  //TODO
}

bool DrmCursor::IsCursorVisible() {
  //TODO
  return false;
}

gfx::PointF DrmCursor::GetLocation() {
  //TODO
  return gfx::PointF();
}

gfx::Rect DrmCursor::GetCursorConfinedBounds() {
  //TODO
  return gfx::Rect();
}

}  // namespace ui
