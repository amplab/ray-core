// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/mojo/drm_gpu_impl.h"

#include "base/file_descriptor_posix.h"
#include "base/files/file_path.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/ozone_drm_gpu/ozone_drm_gpu_type_converters.h"
#include "ui/ozone/public/ozone_platform.h"

namespace ui {

MojoDrmGpuImpl::MojoDrmGpuImpl(
    mojo::InterfaceRequest<mojo::OzoneDrmGpu> request)
    : binding_(this, request.Pass()) {
  ui::OzonePlatform::InitializeForGPU();

  platform_support_ = static_cast<ui::DrmGpuPlatformSupport*>(
      ui::OzonePlatform::GetInstance()->GetGpuPlatformSupport());

  platform_support_->OnChannelEstablished();
}

MojoDrmGpuImpl::~MojoDrmGpuImpl() {}

void MojoDrmGpuImpl::CreateWindow(int64_t widget) {
  platform_support_->OnCreateWindow(widget);
}

void MojoDrmGpuImpl::WindowBoundsChanged(int64_t widget, mojo::RectPtr bounds) {
  platform_support_->OnWindowBoundsChanged(widget, bounds.To<gfx::Rect>());
}

void MojoDrmGpuImpl::AddGraphicsDevice(const mojo::String& file_path,
                                       int32_t file_descriptor) {
  platform_support_->OnAddGraphicsDevice(
      base::FilePath(file_path.get()),
      base::FileDescriptor(file_descriptor, false));
}

void MojoDrmGpuImpl::RefreshNativeDisplays() {
  platform_support_->OnRefreshNativeDisplays();
}

void MojoDrmGpuImpl::ConfigureNativeDisplay(int64_t id,
                                            mojo::DisplayModePtr mode,
                                            mojo::PointPtr originhost) {
  platform_support_->OnConfigureNativeDisplay(
      id, mode.To<ui::DisplayMode_Params>(), originhost.To<gfx::Point>());
}

}  // namespace ui
