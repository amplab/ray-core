// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/thread_task_runner_handle.h"
#include "base/threading/thread.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_platform_support_inprocess.h"
#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host_inprocess.h"
#include "ui/ozone/public/gpu_platform_support.h"
#include "ui/ozone/public/gpu_platform_support_host.h"
#include "ui/ozone/public/ozone_gpu_test_helper.h"
#include "ui/ozone/public/ozone_platform.h"

namespace ui {

namespace {

const int kGpuProcessHostId = 1;

}  // namespace

static void DispatchToGpuPlatformSupportHostTask(Message* msg) {
  auto support = static_cast<DrmGpuPlatformSupportHost*>(
    ui::OzonePlatform::GetInstance()->GetGpuPlatformSupportHost());
  auto inprocess = static_cast<DrmGpuPlatformSupportHostInprocess*>(
    support->get_delegate());
  inprocess->OnMessageReceived(*msg);
  delete msg;
}

class FakeGpuProcess {
 public:
  FakeGpuProcess(
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner)
      : ui_task_runner_(ui_task_runner) {}
  ~FakeGpuProcess() {}

  void Init() {
    base::Callback<void(Message*)> sender =
      base::Bind(&DispatchToGpuPlatformSupportHostTask);

    auto delegate = new DrmGpuPlatformSupportInprocess();
    delegate->OnChannelEstablished(ui_task_runner_, sender);
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
};

static void DispatchToGpuPlatformSupportTask(Message* msg) {
  auto support = static_cast<DrmGpuPlatformSupport*>(
    ui::OzonePlatform::GetInstance()->GetGpuPlatformSupport());
  auto inprocess = static_cast<DrmGpuPlatformSupportInprocess*>(
    support->get_delegate());
  inprocess->OnMessageReceived(*msg);
  delete msg;
}

class FakeGpuProcessHost {
 public:
  FakeGpuProcessHost(
      const scoped_refptr<base::SingleThreadTaskRunner>& gpu_task_runner)
      : gpu_task_runner_(gpu_task_runner) {}
  ~FakeGpuProcessHost() {}

  void Init() {
    base::Callback<void(Message*)> sender =
      base::Bind(&DispatchToGpuPlatformSupportTask);

    auto host_support_inprocess = new DrmGpuPlatformSupportHostInprocess();
    host_support_inprocess->OnChannelEstablished(
      kGpuProcessHostId, gpu_task_runner_, sender);
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;
};

OzoneGpuTestHelper::OzoneGpuTestHelper() {
}

OzoneGpuTestHelper::~OzoneGpuTestHelper() {
}

bool OzoneGpuTestHelper::Initialize(
    const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& gpu_task_runner) {

  fake_gpu_process_.reset(new FakeGpuProcess(ui_task_runner));
  fake_gpu_process_->Init();

  fake_gpu_process_host_.reset(new FakeGpuProcessHost(gpu_task_runner));
  fake_gpu_process_host_->Init();

  return true;
}

}  // namespace ui
