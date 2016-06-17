// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_DRM_CURSOR_H_
#define UI_OZONE_PLATFORM_DRM_HOST_DRM_CURSOR_H_

#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "ui/events/ozone/evdev/cursor_delegate_evdev.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/public/gpu_platform_support_host.h"
#include "ui/platform_window/platform_window.h" // for PlatformCursor

namespace gfx {
class PointF;
class Vector2dF;
class Rect;
}

namespace ui {

class BitmapCursorOzone;
class BitmapCursorFactoryOzone;
class DrmGpuPlatformSupportHost;
class DrmWindowHostManager;

class DrmCursor : public CursorDelegateEvdev {
 public:
  explicit DrmCursor(DrmWindowHostManager* window_manager);
  ~DrmCursor() override;

  // Change the cursor over the specifed window.
  void SetCursor(gfx::AcceleratedWidget window, PlatformCursor platform_cursor);

  // Handle window lifecycle.
  void OnWindowAdded(gfx::AcceleratedWidget window,
                     const gfx::Rect& bounds_in_screen,
                     const gfx::Rect& cursor_confined_bounds);
  void OnWindowRemoved(gfx::AcceleratedWidget window);

  // Handle window bounds changes.
  void CommitBoundsChange(gfx::AcceleratedWidget window,
                          const gfx::Rect& new_display_bounds_in_screen,
                          const gfx::Rect& new_confined_bounds);

  // CursorDelegateEvdev:
  void MoveCursorTo(gfx::AcceleratedWidget window,
                    const gfx::PointF& location) override;
  void MoveCursorTo(const gfx::PointF& screen_location) override;
  void MoveCursor(const gfx::Vector2dF& delta) override;
  bool IsCursorVisible() override;
  gfx::PointF GetLocation() override;
  gfx::Rect GetCursorConfinedBounds() override;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_HOST_DRM_CURSOR_H_
