// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_MOJO_DRM_HOST_DELEGATE_H_
#define UI_OZONE_PLATFORM_DRM_MOJO_DRM_HOST_DELEGATE_H_

#include "base/macros.h"
#include "mojo/services/ozone_drm_gpu/interfaces/ozone_drm_gpu.mojom.h"
#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host.h"

namespace ui {

class MojoDrmHostDelegate : public ui::DrmGpuPlatformSupportHostDelegate {
 public:
  explicit MojoDrmHostDelegate(mojo::OzoneDrmGpu* ozone_drm_gpu);
  ~MojoDrmHostDelegate() override;

 private:
  // DrmGpuPlatformSupportHostDelegate.
  void CreateWindow(const gfx::AcceleratedWidget& widget) override;
  void WindowBoundsChanged(const gfx::AcceleratedWidget& widget,
                           const gfx::Rect& bounds) override;
  void AddGraphicsDevice(const base::FilePath& path,
                         const base::FileDescriptor& fd) override;
  bool RefreshNativeDisplays() override;
  bool ConfigureNativeDisplay(int64_t id,
                              const ui::DisplayMode_Params& mode,
                              const gfx::Point& originhost) override;

  mojo::OzoneDrmGpu* ozone_drm_gpu_;

  DISALLOW_COPY_AND_ASSIGN(MojoDrmHostDelegate);
};

}  // namespace ui

#endif
