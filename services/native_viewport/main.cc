// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application/run_application_options_chromium.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/run_application.h"

#if defined(USE_OZONE)
#include "services/native_viewport/ozone/native_viewport_app_ozone.h"
#else
#include "services/native_viewport/native_viewport_app.h"
#endif

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
#if defined(USE_OZONE)
  native_viewport::NativeViewportAppOzone native_viewport_app;
#else
  native_viewport::NativeViewportApp native_viewport_app;
#endif
  mojo::RunApplicationOptionsChromium options(base::MessageLoop::TYPE_UI);
  return mojo::RunApplication(application_request, &native_viewport_app,
                              &options);
}
