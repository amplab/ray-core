// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/mojo/drm_host_impl.h"

#include "base/logging.h"
#include "mojo/converters/ozone_drm_gpu/ozone_drm_gpu_type_converters.h"
#include "ui/ozone/public/ozone_platform.h"

namespace ui {

static const int kFakeProcessId = -1;

MojoDrmHostImpl::MojoDrmHostImpl(
    mojo::InterfaceRequest<mojo::OzoneDrmHost> request)
    : platform_support_(static_cast<ui::DrmGpuPlatformSupportHost*>(
          ui::OzonePlatform::GetInstance()
              ->GetGpuPlatformSupportHost())),
      binding_(this, request.Pass()) {
  platform_support_->OnChannelEstablished(kFakeProcessId);
}

MojoDrmHostImpl::~MojoDrmHostImpl() {}

void MojoDrmHostImpl::UpdateNativeDisplays(
    mojo::Array<mojo::DisplaySnapshotPtr> displays) {
  platform_support_->get_display_manager()->OnUpdateNativeDisplays(
      displays.To<std::vector<ui::DisplaySnapshot_Params>>());
}

void MojoDrmHostImpl::DisplayConfigured(int64_t id, bool result) {
  platform_support_->get_display_manager()->OnDisplayConfigured(id, result);
}

}  // namespace ui
