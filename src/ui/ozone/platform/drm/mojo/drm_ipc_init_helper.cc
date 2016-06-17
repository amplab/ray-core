// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/ozone_drm_gpu/interfaces/ozone_drm_gpu.mojom.h"
#include "mojo/services/ozone_drm_host/interfaces/ozone_drm_host.mojom.h"
#include "ui/ozone/platform/drm/mojo/drm_gpu_delegate.h"
#include "ui/ozone/platform/drm/mojo/drm_gpu_impl.h"
#include "ui/ozone/platform/drm/mojo/drm_host_delegate.h"
#include "ui/ozone/platform/drm/mojo/drm_host_impl.h"
#include "ui/ozone/public/ipc_init_helper_mojo.h"

namespace ui {

class DrmIpcInitHelperMojo : public IpcInitHelperMojo {
 public:
  DrmIpcInitHelperMojo();
  ~DrmIpcInitHelperMojo();

  void HostInitialize(mojo::Shell* shell) override;
  bool HostAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override;

  void GpuInitialize(mojo::Shell* shell) override;
  bool GpuAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override;

 private:
  mojo::OzoneDrmHostPtr ozone_drm_host_;
  mojo::OzoneDrmGpuPtr ozone_drm_gpu_;
};

DrmIpcInitHelperMojo::DrmIpcInitHelperMojo() {}

DrmIpcInitHelperMojo::~DrmIpcInitHelperMojo() {}

void DrmIpcInitHelperMojo::HostInitialize(mojo::Shell* shell) {
  mojo::ConnectToService(shell, "mojo:native_viewport_service",
                         GetProxy(&ozone_drm_gpu_));
  new MojoDrmHostDelegate(ozone_drm_gpu_.get());
}

void DrmIpcInitHelperMojo::GpuInitialize(mojo::Shell* shell) {
  mojo::ConnectToService(shell, "mojo:native_viewport_service",
                         GetProxy(&ozone_drm_host_));
  new MojoDrmGpuDelegate(ozone_drm_host_.get());
}

bool DrmIpcInitHelperMojo::HostAcceptConnection(
    mojo::ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<mojo::OzoneDrmHost>(
      [](const mojo::ConnectionContext& connection_context,
         mojo::InterfaceRequest<mojo::OzoneDrmHost> request) {
        new MojoDrmHostImpl(request.Pass());
      });
  return true;
}

bool DrmIpcInitHelperMojo::GpuAcceptConnection(
    mojo::ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<mojo::OzoneDrmGpu>(
      [](const mojo::ConnectionContext& connection_context,
         mojo::InterfaceRequest<mojo::OzoneDrmGpu> request) {
        new MojoDrmGpuImpl(request.Pass());
      });
  return true;
}

// static
IpcInitHelperOzone* IpcInitHelperOzone::Create() {
  return new DrmIpcInitHelperMojo();
}

}  // namespace ui
