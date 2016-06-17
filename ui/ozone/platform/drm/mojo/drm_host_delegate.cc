// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/mojo/drm_host_delegate.h"

#include "base/logging.h"
#include "base/process/process.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/ozone_drm_gpu/ozone_drm_gpu_type_converters.h"
#include "ui/ozone/public/ozone_platform.h"

namespace ui {

MojoDrmHostDelegate::MojoDrmHostDelegate(mojo::OzoneDrmGpu* ozone_drm_gpu)
    : ozone_drm_gpu_(ozone_drm_gpu) {
  auto platform_support = static_cast<ui::DrmGpuPlatformSupportHost*>(
      ui::OzonePlatform::GetInstance()->GetGpuPlatformSupportHost());
  platform_support->SetDelegate(this);
}

MojoDrmHostDelegate::~MojoDrmHostDelegate() {}

void MojoDrmHostDelegate::CreateWindow(const gfx::AcceleratedWidget& widget) {
  ozone_drm_gpu_->CreateWindow(widget);
}

void MojoDrmHostDelegate::WindowBoundsChanged(
    const gfx::AcceleratedWidget& widget,
    const gfx::Rect& bounds) {
  ozone_drm_gpu_->WindowBoundsChanged(widget, mojo::Rect::From(bounds));
}

void MojoDrmHostDelegate::AddGraphicsDevice(const base::FilePath& path,
                                            const base::FileDescriptor& fd) {
  ozone_drm_gpu_->AddGraphicsDevice(path.value(), fd.fd);
}

bool MojoDrmHostDelegate::RefreshNativeDisplays() {
  ozone_drm_gpu_->RefreshNativeDisplays();
  return true;
}

bool MojoDrmHostDelegate::ConfigureNativeDisplay(
    int64_t id,
    const ui::DisplayMode_Params& mode,
    const gfx::Point& originhost) {
  ozone_drm_gpu_->ConfigureNativeDisplay(
      id, mojo::DisplayMode::From<ui::DisplayMode_Params>(mode),
      mojo::Point::From(originhost));
  return true;
}

}  // namespace ui
