// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host.h"

#include "base/trace_event/trace_event.h"
#include "ui/ozone/platform/drm/host/channel_observer.h"

namespace ui {

DrmGpuPlatformSupportHost::DrmGpuPlatformSupportHost(DrmCursor* cursor)
    : connected_(false),
      display_manager_(nullptr),
      window_manager_(nullptr),
      cursor_(cursor),
      delegate_(nullptr) {}

DrmGpuPlatformSupportHost::~DrmGpuPlatformSupportHost() {
}

void DrmGpuPlatformSupportHost::SetDisplayManager(
    DrmDisplayHostManager* display_manager) {
  DCHECK(!display_manager_);
  display_manager_ = display_manager;
  if (IsConnected()) {
    display_manager_->OnChannelEstablished();
  }
}

void DrmGpuPlatformSupportHost::SetWindowManager(
    DrmWindowHostManager* window_manager) {
  DCHECK(!window_manager_);
  window_manager_ = window_manager;
}

void DrmGpuPlatformSupportHost::AddChannelObserver(ChannelObserver* observer) {
  channel_observers_.AddObserver(observer);

  if (IsConnected())
    observer->OnChannelEstablished();
}

void DrmGpuPlatformSupportHost::RemoveChannelObserver(
    ChannelObserver* observer) {
  channel_observers_.RemoveObserver(observer);
}

bool DrmGpuPlatformSupportHost::IsConnected() {
  return connected_;
}

void DrmGpuPlatformSupportHost::OnChannelEstablished(int host_id) {
  TRACE_EVENT1("drm", "DrmGpuPlatformSupportHost::OnChannelEstablished",
               "host_id", host_id);

  connected_ = true;

  if (display_manager_) {
    display_manager_->OnChannelEstablished();
  }

  FOR_EACH_OBSERVER(ChannelObserver, channel_observers_,
                    OnChannelEstablished());
}

void DrmGpuPlatformSupportHost::OnChannelDestroyed(int host_id) {
   TRACE_EVENT1("drm", "DrmGpuPlatformSupportHost::OnChannelDestroyed",
                "host_id", host_id);

   if (connected_) {
     connected_ = false;
     FOR_EACH_OBSERVER(ChannelObserver, channel_observers_,
                       OnChannelDestroyed());
   }
}

void DrmGpuPlatformSupportHost::CreateWindow(
    const gfx::AcceleratedWidget& widget) {
  DCHECK(delegate_);
  delegate_->CreateWindow(widget);
}

void DrmGpuPlatformSupportHost::DestroyWindow(
    const gfx::AcceleratedWidget& widget) {
  NOTIMPLEMENTED();
}

void DrmGpuPlatformSupportHost::WindowBoundsChanged(
    const gfx::AcceleratedWidget& widget,
    const gfx::Rect& bounds) {
  DCHECK(delegate_);
  delegate_->WindowBoundsChanged(widget, bounds);
}

void DrmGpuPlatformSupportHost::AddGraphicsDevice(
    const base::FilePath& path,
    const base::FileDescriptor& fd) {
  DCHECK(delegate_);
  delegate_->AddGraphicsDevice(path, fd);
}

void DrmGpuPlatformSupportHost::RemoveGraphicsDevice(
    const base::FilePath& path) {
  NOTIMPLEMENTED();
}

bool DrmGpuPlatformSupportHost::RefreshNativeDisplays() {
  DCHECK(delegate_);
  return delegate_->RefreshNativeDisplays();
}

bool DrmGpuPlatformSupportHost::ConfigureNativeDisplay(
    int64_t id, const DisplayMode_Params& mode, const gfx::Point& originhost) {
  DCHECK(delegate_);
  return delegate_->ConfigureNativeDisplay(id, mode, originhost);
}

bool DrmGpuPlatformSupportHost::DisableNativeDisplay(int64_t id) {
  NOTIMPLEMENTED();
  return true;
}

void DrmGpuPlatformSupportHost::CheckOverlayCapabilities(
  const gfx::AcceleratedWidget& widget,
  const std::vector<OverlayCheck_Params>& list) {
  NOTIMPLEMENTED();
}

bool DrmGpuPlatformSupportHost::GetHDCPState(int64_t id) {
  NOTIMPLEMENTED();
  return true;
}

bool DrmGpuPlatformSupportHost::SetHDCPState(int64_t id,
                                             const HDCPState& state) {
  NOTIMPLEMENTED();
  return true;
}

bool DrmGpuPlatformSupportHost::TakeDisplayControl() {
  NOTIMPLEMENTED();
  return true;
}

bool DrmGpuPlatformSupportHost::RelinquishDisplayControl() {
  NOTIMPLEMENTED();
  return true;
}

bool DrmGpuPlatformSupportHost::SetGammaRamp(
    int64_t id, const std::vector<GammaRampRGBEntry>& lut) {
  NOTIMPLEMENTED();
  return true;
}

}  // namespace ui
