// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note: This file also partly tests child_main.*.

#include "shell/child_process_host.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "build/build_config.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "shell/application_manager/native_application_options.h"
#include "shell/context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace shell {
namespace {

// Subclass just so we can observe |DidStart()|.
class TestChildProcessHost : public ChildProcessHost {
 public:
  explicit TestChildProcessHost(Context* context) : ChildProcessHost(context) {}
  ~TestChildProcessHost() override {}

  void DidStart(base::Process child_process) override {
    EXPECT_TRUE(child_process.IsValid());
    ChildProcessHost::DidStart(child_process.Pass());
    base::MessageLoop::current()->QuitWhenIdle();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestChildProcessHost);
};

#if defined(OS_ANDROID)
// TODO(qsr): Multiprocess shell tests are not supported on android.
#define MAYBE_StartJoin DISABLED_StartJoin
#else
#define MAYBE_StartJoin StartJoin
#endif  // defined(OS_ANDROID)
// Just tests starting the child process and joining it (without starting an
// app).
TEST(ChildProcessHostTest, MAYBE_StartJoin) {
  Context context;
  base::MessageLoop message_loop(
      scoped_ptr<base::MessagePump>(new mojo::common::MessagePumpMojo()));
  context.Init();
  TestChildProcessHost child_process_host(&context);
  child_process_host.Start(NativeApplicationOptions());
  message_loop.Run();  // This should run until |DidStart()|.
  child_process_host.ExitNow(123);
  int exit_code = child_process_host.Join();
  VLOG(2) << "Joined child: exit_code = " << exit_code;
  EXPECT_EQ(123, exit_code);

  context.Shutdown();
}

#if defined(OS_ANDROID)
// TODO(qsr): Multiprocess shell tests are not supported on android.
#define MAYBE_ConnectionError DISABLED_ConnectionError
#else
#define MAYBE_ConnectionError ConnectionError
#endif  // defined(OS_ANDROID)
// Tests that even on connection error, the callback to |StartApp()| will get
// called.
TEST(ChildProcessHostTest, MAYBE_ConnectionError) {
  Context context;
  base::MessageLoop message_loop(
      scoped_ptr<base::MessagePump>(new mojo::common::MessagePumpMojo()));
  context.Init();
  TestChildProcessHost child_process_host(&context);
  child_process_host.Start(NativeApplicationOptions());
  message_loop.Run();  // This should run until |DidStart()|.
  // Send |ExitNow()| first, so that the |StartApp()| below won't actually be
  // processed, and we'll just get a connection error.
  child_process_host.ExitNow(123);
  mojo::MessagePipe mp;
  mojo::InterfaceRequest<mojo::Application> application_request;
  application_request.Bind(mp.handle0.Pass());
  // This won't actually be called, but the callback should be run.
  MojoResult result = MOJO_RESULT_INTERNAL;
  child_process_host.StartApp("/does_not_exist/cbvgyuio",
                              application_request.Pass(), [&result](int32_t r) {
                                result = r;
                                base::MessageLoop::current()->QuitWhenIdle();
                              });
  message_loop.Run();
  EXPECT_EQ(MOJO_RESULT_UNKNOWN, result);
  int exit_code = child_process_host.Join();
  VLOG(2) << "Joined child: exit_code = " << exit_code;
  EXPECT_EQ(123, exit_code);

  context.Shutdown();
}

}  // namespace
}  // namespace shell
