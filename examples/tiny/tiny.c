// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/system/main.h"
#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"

MojoResult MojoMain(MojoHandle app_request) {
  return MojoClose(app_request);
}
