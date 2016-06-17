// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/context.h"

#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "build/build_config.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace shell {
namespace {

#if defined(OS_ANDROID)
// TODO(vtl): The command line doesn't get initialized in a reasonable way on
// Android, currently. (Relatedly, multiprocess tests are currently not
// supported.)
#define MAYBE_Paths DISABLED_Paths
#else
#define MAYBE_Paths Paths
#endif
TEST(ContextTest, MAYBE_Paths) {
  Context context;
  base::MessageLoop message_loop(
      scoped_ptr<base::MessagePump>(new mojo::common::MessagePumpMojo()));
  context.Init();

  EXPECT_FALSE(context.mojo_shell_child_path().empty());
  EXPECT_TRUE(context.mojo_shell_child_path().IsAbsolute());

  context.Shutdown();
}

}  // namespace
}  // namespace shell
