// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_PLATFORM_SUPPORT_INPROCESS_H_
#define UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_PLATFORM_SUPPORT_INPROCESS_H_

#include "base/callback.h"
#include "base/single_thread_task_runner.h"
#include "ui/ozone/platform/drm/common/inprocess_messages.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_platform_support.h"

namespace ui {

class DrmGpuPlatformSupportInprocess : public DrmGpuPlatformSupportDelegate {
 public:
  DrmGpuPlatformSupportInprocess();
  ~DrmGpuPlatformSupportInprocess() override;

  void OnChannelEstablished(
    scoped_refptr<base::SingleThreadTaskRunner> send_runner,
    base::Callback<void(Message*)> send_callback);

  bool OnMessageReceived(const Message& message);
  bool Send(Message* message);

  void UpdateNativeDisplays(
    const std::vector<DisplaySnapshot_Params>& displays) override;
  void DisplayConfigured(int64_t id, bool result) override;

 private:
  DrmGpuPlatformSupport* platform_support_;
  scoped_refptr<base::SingleThreadTaskRunner> send_runner_;
  base::Callback<void(Message*)> send_callback_;
};

} // namespace

#endif
