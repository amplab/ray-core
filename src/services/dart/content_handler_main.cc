// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/run_application.h"
#include "services/dart/content_handler_app.h"

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  dart::DartContentHandlerApp dart_content_handler_app;
  return mojo::RunApplication(application_request, &dart_content_handler_app);
}
