// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_VIEWPORT_OZONE_NATIVE_VIEWPORT_APP_OZONE_H_
#define SERVICES_NATIVE_VIEWPORT_OZONE_NATIVE_VIEWPORT_APP_OZONE_H_

#include "services/native_viewport/native_viewport_app.h"
#include "services/native_viewport/ozone/display_manager.h"

namespace native_viewport {

class NativeViewportAppOzone : public NativeViewportApp {
 public:
  using NativeViewportApp::Create;

  void OnInitialize() override;
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override;

 private:
  std::unique_ptr<DisplayManager> display_manager_;
};

}  // namespace native_viewport

#endif  // SERVICES_NATIVE_VIEWPORT_OZONE_NATIVE_VIEWPORT_APP_OZONE_H_
