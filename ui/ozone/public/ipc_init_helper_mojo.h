// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PUBLIC_IPC_INIT_HELPER_MOJO_H_
#define UI_OZONE_PUBLIC_IPC_INIT_HELPER_MOJO_H_

#include "ui/ozone/public/ipc_init_helper_ozone.h"

namespace mojo {
class ServiceProviderImpl;
class Shell;
}  // namespace mojo

namespace ui {

class IpcInitHelperMojo : public IpcInitHelperOzone {
 public:
  virtual void HostInitialize(mojo::Shell* shell) = 0;
  virtual bool HostAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) = 0;

  virtual void GpuInitialize(mojo::Shell* shell) = 0;
  virtual bool GpuAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) = 0;
};

}  // namespace ui

#endif  // UI_OZONE_PUBLIC_IPC_INIT_HELPER_MOJO_H_
