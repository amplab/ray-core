// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_SYSTEM_MAIN_H_
#define MOJO_PUBLIC_C_SYSTEM_MAIN_H_

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/macros.h"
#include "mojo/public/c/system/result.h"

// Implement MojoMain directly as the entry point for an application.
//
// MojoResult MojoMain(MojoHandle application_request) {
//   ...
// }

MOJO_BEGIN_EXTERN_C

__attribute__((visibility("default"))) MojoResult MojoMain(
    MojoHandle application_request);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_SYSTEM_MAIN_H_
