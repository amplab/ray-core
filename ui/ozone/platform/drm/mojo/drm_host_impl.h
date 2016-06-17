// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_MOJO_DRM_HOST_IMPL_H_
#define UI_OZONE_PLATFORM_DRM_MOJO_DRM_HOST_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/ozone_drm_host/interfaces/ozone_drm_host.mojom.h"
#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host.h"

namespace ui {

class MojoDrmHostImpl : public mojo::OzoneDrmHost {
 public:
  explicit MojoDrmHostImpl(mojo::InterfaceRequest<mojo::OzoneDrmHost> request);
  ~MojoDrmHostImpl() override;

 private:
  // OzoneDrmHost
  void UpdateNativeDisplays(
      mojo::Array<mojo::DisplaySnapshotPtr> displays) override;
  void DisplayConfigured(int64_t id, bool result) override;

  ui::DrmGpuPlatformSupportHost* platform_support_;
  mojo::StrongBinding<mojo::OzoneDrmHost> binding_;

  DISALLOW_COPY_AND_ASSIGN(MojoDrmHostImpl);
};

}  // namespace ui

#endif
