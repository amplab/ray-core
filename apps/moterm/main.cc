// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is the "main" for the embeddable Moterm terminal view, which provides
// services to the thing embedding it. (This is not very useful as a "top-level"
// application.)

#include "apps/moterm/moterm_app.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/run_application.h"

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  MotermApp moterm_app;
  return mojo::RunApplication(application_request, &moterm_app);
}
