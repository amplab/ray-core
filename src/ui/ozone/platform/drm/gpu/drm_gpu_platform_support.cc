// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/drm_gpu_platform_support.h"

#include "base/bind.h"
#include "base/thread_task_runner_handle.h"
#include "ui/ozone/platform/drm/gpu/drm_device.h"
#include "ui/ozone/platform/drm/gpu/drm_device_manager.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_display_manager.h"
#include "ui/ozone/platform/drm/gpu/drm_window.h"
#include "ui/ozone/platform/drm/gpu/scanout_buffer.h"
#include "ui/ozone/platform/drm/gpu/screen_manager.h"

namespace ui {

DrmGpuPlatformSupport::DrmGpuPlatformSupport(
    DrmDeviceManager* drm_device_manager,
    ScreenManager* screen_manager,
    ScanoutBufferGenerator* buffer_generator,
    scoped_ptr<DrmGpuDisplayManager> display_manager)
    : drm_device_manager_(drm_device_manager),
      screen_manager_(screen_manager),
      display_manager_(display_manager.Pass()),
      delegate_(nullptr) {
}

DrmGpuPlatformSupport::~DrmGpuPlatformSupport() {
}

void DrmGpuPlatformSupport::SetDelegate(
    DrmGpuPlatformSupportDelegate* delegate) {
  DCHECK(!delegate_);
  delegate_ = delegate;
}

void DrmGpuPlatformSupport::AddHandler(scoped_ptr<GpuPlatformSupport> handler) {
  handlers_.push_back(handler.Pass());
}

void DrmGpuPlatformSupport::OnChannelEstablished() {
  for (size_t i = 0; i < handlers_.size(); ++i)
    handlers_[i]->OnChannelEstablished();
}

void DrmGpuPlatformSupport::OnCreateWindow(gfx::AcceleratedWidget widget) {
  scoped_ptr<DrmWindow> delegate(
      new DrmWindow(widget, drm_device_manager_, screen_manager_));
  delegate->Initialize();
  screen_manager_->AddWindow(widget, delegate.Pass());
}

void DrmGpuPlatformSupport::OnDestroyWindow(gfx::AcceleratedWidget widget) {
  scoped_ptr<DrmWindow> delegate = screen_manager_->RemoveWindow(widget);
  delegate->Shutdown();
}

void DrmGpuPlatformSupport::OnWindowBoundsChanged(gfx::AcceleratedWidget widget,
                                                  const gfx::Rect& bounds) {
  screen_manager_->GetWindow(widget)->OnBoundsChanged(bounds);
}

void DrmGpuPlatformSupport::OnRefreshNativeDisplays() {
  DCHECK(delegate_);
  delegate_->UpdateNativeDisplays(display_manager()->GetDisplays());
}

void DrmGpuPlatformSupport::OnCursorSet(gfx::AcceleratedWidget widget,
                                        const std::vector<SkBitmap>& bitmaps,
                                        const gfx::Point& location,
                                        int frame_delay_ms) {
  screen_manager_->GetWindow(widget)
      ->SetCursor(bitmaps, location, frame_delay_ms);
}

void DrmGpuPlatformSupport::OnCursorMove(gfx::AcceleratedWidget widget,
                                         const gfx::Point& location) {
  screen_manager_->GetWindow(widget)->MoveCursor(location);
}

void DrmGpuPlatformSupport::OnCheckOverlayCapabilities(
    gfx::AcceleratedWidget widget,
    const std::vector<OverlayCheck_Params>& overlays) {
  NOTIMPLEMENTED();
}

void DrmGpuPlatformSupport::OnConfigureNativeDisplay(
    int64_t id,
    const DisplayMode_Params& mode_param,
    const gfx::Point& origin) {
  DCHECK(delegate_);
  delegate_->DisplayConfigured(
    id, display_manager_->ConfigureDisplay(id, mode_param, origin));
}

void DrmGpuPlatformSupport::OnDisableNativeDisplay(int64_t id) {
  NOTIMPLEMENTED();
}

void DrmGpuPlatformSupport::OnTakeDisplayControl() {
  NOTIMPLEMENTED();
}

void DrmGpuPlatformSupport::OnRelinquishDisplayControl() {
  NOTIMPLEMENTED();
}

void DrmGpuPlatformSupport::OnAddGraphicsDevice(
    const base::FilePath& path,
    const base::FileDescriptor& fd) {
  drm_device_manager_->AddDrmDevice(path, fd);
}

void DrmGpuPlatformSupport::OnRemoveGraphicsDevice(const base::FilePath& path) {
  drm_device_manager_->RemoveDrmDevice(path);
}

void DrmGpuPlatformSupport::OnSetGammaRamp(
    int64_t id,
    const std::vector<GammaRampRGBEntry>& lut) {
  display_manager_->SetGammaRamp(id, lut);
}

void DrmGpuPlatformSupport::OnGetHDCPState(int64_t display_id) {
  NOTIMPLEMENTED();
}

void DrmGpuPlatformSupport::OnSetHDCPState(int64_t display_id,
                                           HDCPState state) {
  NOTIMPLEMENTED();
}

void DrmGpuPlatformSupport::SetIOTaskRunner(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner) {
  drm_device_manager_->InitializeIOTaskRunner(io_task_runner);
}

}  // namespace ui
