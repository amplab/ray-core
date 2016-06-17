// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_PLATFORM_SUPPORT_H_
#define UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_PLATFORM_SUPPORT_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "ui/display/types/display_constants.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/public/gpu_platform_support.h"

class SkBitmap;

namespace base {
class FilePath;
class SingleThreadTaskRunner;
struct FileDescriptor;
}

namespace gfx {
class Point;
class Rect;
}

namespace ui {

class DrmDeviceManager;
class DrmGpuDisplayManager;
class DrmSurfaceFactory;
class DrmWindow;
class ScreenManager;
class ScanoutBufferGenerator;

struct DisplayMode_Params;
struct DisplaySnapshot_Params;
struct OverlayCheck_Params;
struct GammaRampRGBEntry;

class DrmGpuPlatformSupportDelegate {
public:
  DrmGpuPlatformSupportDelegate() {};
  virtual ~DrmGpuPlatformSupportDelegate() {};

  virtual void UpdateNativeDisplays(
    const std::vector<DisplaySnapshot_Params>& displays) = 0;
  virtual void DisplayConfigured(int64_t id, bool result) = 0;
};

class DrmGpuPlatformSupport : public GpuPlatformSupport {
 public:
  DrmGpuPlatformSupport(DrmDeviceManager* drm_device_manager,
                        ScreenManager* screen_manager,
                        ScanoutBufferGenerator* buffer_generator,
                        scoped_ptr<DrmGpuDisplayManager> display_manager);
  ~DrmGpuPlatformSupport() override;

  void SetDelegate(DrmGpuPlatformSupportDelegate* delegate);

  DrmGpuPlatformSupportDelegate* get_delegate() const {
    return delegate_;
  }

  void AddHandler(scoped_ptr<GpuPlatformSupport> handler);

  // GpuPlatformSupport:
  void OnChannelEstablished() override;

  void OnCreateWindow(gfx::AcceleratedWidget widget);
  void OnDestroyWindow(gfx::AcceleratedWidget widget);
  void OnWindowBoundsChanged(gfx::AcceleratedWidget widget,
                             const gfx::Rect& bounds);
  void OnCursorSet(gfx::AcceleratedWidget widget,
                   const std::vector<SkBitmap>& bitmaps,
                   const gfx::Point& location,
                   int frame_delay_ms);
  void OnCursorMove(gfx::AcceleratedWidget widget, const gfx::Point& location);
  void OnCheckOverlayCapabilities(
      gfx::AcceleratedWidget widget,
      const std::vector<OverlayCheck_Params>& overlays);

  DrmGpuDisplayManager* display_manager() {
    return display_manager_.get();
  }

  void OnRefreshNativeDisplays();
  void OnConfigureNativeDisplay(int64_t id,
                                const DisplayMode_Params& mode,
                                const gfx::Point& origin);
  void OnDisableNativeDisplay(int64_t id);
  void OnTakeDisplayControl();
  void OnRelinquishDisplayControl();
  void OnAddGraphicsDevice(const base::FilePath& path,
                           const base::FileDescriptor& fd);
  void OnRemoveGraphicsDevice(const base::FilePath& path);
  void OnGetHDCPState(int64_t display_id);
  void OnSetHDCPState(int64_t display_id, HDCPState state);
  void OnSetGammaRamp(int64_t id, const std::vector<GammaRampRGBEntry>& lut);

  void SetIOTaskRunner(
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);

  DrmDeviceManager* drm_device_manager_;  // Not owned.
  ScreenManager* screen_manager_;         // Not owned.

  scoped_ptr<DrmGpuDisplayManager> display_manager_;
  ScopedVector<GpuPlatformSupport> handlers_;
  DrmGpuPlatformSupportDelegate* delegate_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_PLATFORM_SUPPORT_H_
