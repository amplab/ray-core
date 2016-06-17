// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/android_handler_loader.h"

namespace shell {

AndroidHandlerLoader::AndroidHandlerLoader() {}

AndroidHandlerLoader::~AndroidHandlerLoader() {}

void AndroidHandlerLoader::Load(
    const GURL& url,
    mojo::InterfaceRequest<mojo::Application> application_request) {
  DCHECK(application_request.is_pending());
  android_handler_.Bind(application_request.Pass());
}

}  // namespace shell
