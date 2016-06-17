// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is used to ensure that our C headers can be compiled as C++ with
// more stringent warnings, in particular with "-Wundef".

// Include all the header files that are meant to be compilable as C.
#include "mojo/public/c/environment/async_waiter.h"
#include "mojo/public/c/environment/logger.h"
#include "mojo/public/c/system/buffer.h"
#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/macros.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/c/system/time.h"
#include "mojo/public/c/system/wait.h"
#include "mojo/public/c/system/wait_set.h"

// We don't actually want to test anything; we just want to make sure that this
// file is compiled/linked in (this function is called from core_unittest.cc).
const char* MinimalCppTest() {
  return nullptr;
}
