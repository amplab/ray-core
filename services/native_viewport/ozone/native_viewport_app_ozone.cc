// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_viewport/ozone/native_viewport_app_ozone.h"
#include "ui/ozone/public/ipc_init_helper_mojo.h"
#include "ui/ozone/public/ozone_platform.h"

namespace native_viewport {

void NativeViewportAppOzone::OnInitialize() {
  NativeViewportApp::OnInitialize();

  ui::OzonePlatform::InitializeForUI();

  auto ipc_init_helper = static_cast<ui::IpcInitHelperMojo*>(
      ui::OzonePlatform::GetInstance()->GetIpcInitHelperOzone());

  ipc_init_helper->HostInitialize(shell());
  ipc_init_helper->GpuInitialize(shell());

  display_manager_.reset(new DisplayManager());
}

bool NativeViewportAppOzone::OnAcceptConnection(
    ServiceProviderImpl* service_provider_impl) {
  if (!NativeViewportApp::OnAcceptConnection(service_provider_impl))
    return false;

  auto ipc_init_helper = static_cast<ui::IpcInitHelperMojo*>(
      ui::OzonePlatform::GetInstance()->GetIpcInitHelperOzone());

  ipc_init_helper->HostAcceptConnection(service_provider_impl);
  ipc_init_helper->GpuAcceptConnection(service_provider_impl);

  return true;
}

}  // namespace native_viewport
