// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_MOJO_DRM_GPU_DELEGATE_H_
#define UI_OZONE_PLATFORM_DRM_MOJO_DRM_GPU_DELEGATE_H_

#include "base/macros.h"
#include "mojo/services/ozone_drm_host/interfaces/ozone_drm_host.mojom.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_platform_support.h"

namespace ui {

class MojoDrmGpuDelegate : public ui::DrmGpuPlatformSupportDelegate {
 public:
  explicit MojoDrmGpuDelegate(mojo::OzoneDrmHost* ozone_drm_host);
  ~MojoDrmGpuDelegate() override;

 private:
  // DrmGpuPlatformSupportDelegate
  void UpdateNativeDisplays(
      const std::vector<ui::DisplaySnapshot_Params>& displays) override;
  void DisplayConfigured(int64_t id, bool result) override;

  mojo::OzoneDrmHost* ozone_drm_host_;

  DISALLOW_COPY_AND_ASSIGN(MojoDrmGpuDelegate);
};

}  // namespace ui

#endif
