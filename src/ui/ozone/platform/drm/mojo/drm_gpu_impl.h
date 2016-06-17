// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_MOJO_DRM_GPU_IMPL_H_
#define UI_OZONE_PLATFORM_DRM_MOJO_DRM_GPU_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/ozone_drm_gpu/interfaces/ozone_drm_gpu.mojom.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_platform_support.h"

namespace ui {

class MojoDrmGpuImpl : public mojo::OzoneDrmGpu {
 public:
  explicit MojoDrmGpuImpl(mojo::InterfaceRequest<mojo::OzoneDrmGpu> request);
  ~MojoDrmGpuImpl() override;

 private:
  // OzoneDrmGpu.
  void CreateWindow(int64_t widget) override;
  void WindowBoundsChanged(int64_t widget, mojo::RectPtr bounds) override;

  void AddGraphicsDevice(const mojo::String& file_path,
                         int32_t file_descriptor) override;
  void RefreshNativeDisplays() override;
  void ConfigureNativeDisplay(int64_t id,
                              mojo::DisplayModePtr mode,
                              mojo::PointPtr originhost) override;

  ui::DrmGpuPlatformSupport* platform_support_;
  mojo::StrongBinding<mojo::OzoneDrmGpu> binding_;

  DISALLOW_COPY_AND_ASSIGN(MojoDrmGpuImpl);
};

}  // namespace ui

#endif
