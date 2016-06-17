// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ui/noodles/noodles_app.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/run_application.h"

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  examples::NoodlesApp noodles_app;
  return mojo::RunApplication(application_request, &noodles_app);
}
