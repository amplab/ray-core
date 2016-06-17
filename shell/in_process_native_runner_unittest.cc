// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/in_process_native_runner.h"

#include "shell/context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace shell {

TEST(InProcessNativeRunnerTest, NotStarted) {
  Context context;
  base::MessageLoop loop;
  context.Init();
  InProcessNativeRunner runner(&context);
  context.Shutdown();
  // Shouldn't crash or DCHECK on destruction.
}

}  // namespace shell
