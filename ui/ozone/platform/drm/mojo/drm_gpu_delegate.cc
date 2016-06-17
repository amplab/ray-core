// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/mojo/drm_gpu_delegate.h"

#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/ozone_drm_gpu/ozone_drm_gpu_type_converters.h"
#include "ui/ozone/public/ozone_platform.h"

namespace ui {

MojoDrmGpuDelegate::MojoDrmGpuDelegate(mojo::OzoneDrmHost* ozone_drm_host)
    : ozone_drm_host_(ozone_drm_host) {
  ui::OzonePlatform::InitializeForGPU();

  auto platform_support = static_cast<ui::DrmGpuPlatformSupport*>(
      ui::OzonePlatform::GetInstance()->GetGpuPlatformSupport());

  platform_support->SetDelegate(this);
}

MojoDrmGpuDelegate::~MojoDrmGpuDelegate() {}

void MojoDrmGpuDelegate::UpdateNativeDisplays(
    const std::vector<ui::DisplaySnapshot_Params>& displays) {
  ozone_drm_host_->UpdateNativeDisplays(
      mojo::Array<mojo::DisplaySnapshotPtr>::From(displays));
}

void MojoDrmGpuDelegate::DisplayConfigured(int64_t id, bool result) {
  ozone_drm_host_->DisplayConfigured(id, result);
}

}  // namespace ui
