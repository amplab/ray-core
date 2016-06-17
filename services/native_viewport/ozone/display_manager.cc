// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/location.h"
#include "base/thread_task_runner_handle.h"
#include "services/native_viewport/ozone/display_manager.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host.h"
#include "ui/ozone/platform/drm/host/drm_window_host.h"
#include "ui/ozone/public/ozone_platform.h"

namespace native_viewport {

DisplayManager::DisplayManager()
    : delegate_(ui::OzonePlatform::GetInstance()
                    ->CreateNativeDisplayDelegate()),
      is_configuring_(false),
      should_configure_(false),
      weak_factory_(this) {
  delegate_->AddObserver(this);
  delegate_->Initialize();
  OnConfigurationChanged();
}

DisplayManager::~DisplayManager() {
  if (delegate_)
    delegate_->RemoveObserver(this);
}

void DisplayManager::OnConfigurationChanged() {
  VLOG(1) << "DisplayManager::OnConfigurationChanged is_configuring_ "
          << is_configuring_;
  if (is_configuring_) {
    should_configure_ = true;
    return;
  }

  is_configuring_ = true;
  delegate_->GrabServer();
  delegate_->GetDisplays(base::Bind(&DisplayManager::OnDisplaysAcquired,
                                    weak_factory_.GetWeakPtr()));
}

void DisplayManager::OnDisplaysAcquired(
    const std::vector<ui::DisplaySnapshot*>& displays) {
  gfx::Point origin;
  VLOG(1) << "OnDisplaysAcquired displays " << displays.size();

  for (auto display : displays) {
    if (!display->native_mode()) {
      LOG(ERROR) << "Display " << display->display_id()
                 << " doesn't have a native mode";
      continue;
    }

    delegate_->Configure(
        *display, display->native_mode(), origin,
        base::Bind(&DisplayManager::OnDisplayConfigured,
                   weak_factory_.GetWeakPtr(),
                   gfx::Rect(origin, display->native_mode()->size())));
    origin.Offset(display->native_mode()->size().width(), 0);
  }
  delegate_->UngrabServer();
  is_configuring_ = false;

  VLOG(1) << "OnDisplaysAcquired should_configure_ " << should_configure_;
  if (should_configure_) {
    should_configure_ = false;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&DisplayManager::OnConfigurationChanged,
                              base::Unretained(this)));
  }
}

void DisplayManager::OnDisplayConfigured(const gfx::Rect& bounds,
                                         bool success) {
  auto platform_support_host = static_cast<ui::DrmGpuPlatformSupportHost*>(
      ui::OzonePlatform::GetInstance()->GetGpuPlatformSupportHost());
  platform_support_host->get_window_manager()->GetPrimaryWindow()->SetBounds(
      bounds);
}

}  // namespace native_viewport
