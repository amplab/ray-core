// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_DRM_GPU_PLATFORM_SUPPORT_HOST_INPROCESS_H_
#define UI_OZONE_PLATFORM_DRM_HOST_DRM_GPU_PLATFORM_SUPPORT_HOST_INPROCESS_H_

#include "base/single_thread_task_runner.h"
#include "ui/ozone/platform/drm/common/inprocess_messages.h"
#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host.h"

namespace ui {

class DrmGpuPlatformSupportHostInprocess :
    public DrmGpuPlatformSupportHostDelegate {
 public:
  DrmGpuPlatformSupportHostInprocess();
  ~DrmGpuPlatformSupportHostInprocess() override;

  void OnChannelEstablished(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> send_runner,
      const base::Callback<void(Message*)>& send_callback);
  void OnChannelDestroyed(int host_id);

  bool OnMessageReceived(const Message& message);
  bool Send(Message* message);

  void CreateWindow(const gfx::AcceleratedWidget& widget) override;
  void WindowBoundsChanged(const gfx::AcceleratedWidget& widget,
                           const gfx::Rect& bounds) override;
  void AddGraphicsDevice(const base::FilePath& primary_graphics_card_path,
                         const base::FileDescriptor& fd) override;
  bool RefreshNativeDisplays() override;
  bool ConfigureNativeDisplay(int64_t id,
                              const DisplayMode_Params& mode,
                              const gfx::Point& originhost) override;

 private:
  DrmGpuPlatformSupportHost* platform_support_;
  scoped_refptr<base::SingleThreadTaskRunner> send_runner_;
  base::Callback<void(Message*)> send_callback_;
};

}  // namespace ui

#endif  // UI_OZONE_GPU_DRM_GPU_PLATFORM_SUPPORT_HOST_INPROCESS_H_
