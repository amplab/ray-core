// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/child_process_host.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/posix/global_descriptors.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/task_runner.h"
#include "base/task_runner_util.h"
#include "base/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "mojo/edk/base_edk/platform_task_runner_impl.h"
#include "mojo/edk/embedder/multiprocess_embedder.h"
#include "mojo/edk/platform/platform_pipe.h"
#include "mojo/edk/util/ref_ptr.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "shell/application_manager/native_application_options.h"
#include "shell/child_switches.h"
#include "shell/context.h"
#include "shell/task_runners.h"

#include <iostream>
#include "examples/hello_mojo/exchange_file_descriptor.h"

using mojo::platform::PlatformPipe;
using mojo::util::MakeRefCounted;

namespace shell {

struct ChildProcessHost::LaunchData {
  LaunchData() {}
  ~LaunchData() {}

  NativeApplicationOptions options;
  base::FilePath child_path;
  PlatformPipe platform_pipe;
  std::string child_connection_id;
};

ChildProcessHost::ChildProcessHost(Context* context)
    : context_(context), channel_info_(nullptr), external_process_(false) {
}

ChildProcessHost::~ChildProcessHost() {
  DCHECK(!child_process_.IsValid());
}

void ChildProcessHost::Start(const NativeApplicationOptions& options) {
  DCHECK(!child_process_.IsValid());

  scoped_ptr<LaunchData> launch_data(new LaunchData());
  launch_data->options = options;
  launch_data->child_path = context_->mojo_shell_child_path();
#if defined(ARCH_CPU_64_BITS)
  if (options.require_32_bit) {
    launch_data->child_path =
        context_->mojo_shell_child_path().InsertBeforeExtensionASCII("_32");
  }
#endif
  // TODO(vtl): Add something for |slave_info|.
  // TODO(vtl): The "unretained this" is wrong (see also below).
  // TODO(vtl): We shouldn't have to make a new
  // |base_edk::PlatformTaskRunnerImpl| for each instance. Instead, there should
  // be one per thread.
  mojo::ScopedMessagePipeHandle handle(mojo::embedder::ConnectToSlave(
      nullptr, launch_data->platform_pipe.handle0.Pass(),
      [this]() { DidConnectToSlave(); },
      MakeRefCounted<base_edk::PlatformTaskRunnerImpl>(
          base::ThreadTaskRunnerHandle::Get()),
      &launch_data->child_connection_id, &channel_info_));
  // TODO(vtl): We should destroy the channel on destruction (using
  // |channel_info_|, but only after the callback has been called.
  CHECK(channel_info_);

  std::cout << context_->mojo_shell_child_path().value() << std::endl;
  std::cout << "Do you want to infuse a file handle?" << std::endl;
  int answer = 0;
  std::cin >> answer;

  controller_.Bind(mojo::InterfaceHandle<ChildController>(handle.Pass(), 0u));
  if(answer == 1) {
    std::cout << launch_data->child_connection_id << std::endl;
    FileDescriptorSender sender("/home/pcmoritz/server");
    sender.Send(launch_data->platform_pipe.handle1.Pass().get().fd);
    external_process_ = true;
  } else {
    controller_.set_connection_error_handler([this]() { OnConnectionError(); });

    CHECK(base::PostTaskAndReplyWithResult(
      context_->task_runners()->blocking_pool(), FROM_HERE,
      base::Bind(&ChildProcessHost::DoLaunch, base::Unretained(this),
                 base::Passed(&launch_data)),
      base::Bind(&ChildProcessHost::DidStart, base::Unretained(this))));
  }
}

int ChildProcessHost::Join() {
  if (external_process_) {
    return 0;
  }
  DCHECK(child_process_.IsValid());
  int rv = -1;
  LOG_IF(ERROR, !child_process_.WaitForExit(&rv))
      << "Failed to wait for child process";
  child_process_.Close();
  return rv;
}

void ChildProcessHost::StartApp(
    const mojo::String& app_path,
    mojo::InterfaceRequest<mojo::Application> application_request,
    const ChildController::StartAppCallback& on_app_complete) {
  DCHECK(controller_);

  on_app_complete_ = on_app_complete;
  controller_->StartApp(
      app_path, application_request.Pass(),
      base::Bind(&ChildProcessHost::AppCompleted, base::Unretained(this)));
}

void ChildProcessHost::ExitNow(int32_t exit_code) {
  DCHECK(controller_);

  controller_->ExitNow(exit_code);
}

void ChildProcessHost::DidStart(base::Process child_process) {
  DVLOG(2) << "ChildProcessHost::DidStart()";
  DCHECK(!child_process_.IsValid());

  if (!child_process.IsValid()) {
    LOG(ERROR) << "Failed to start app child process";
    AppCompleted(MOJO_RESULT_UNKNOWN);
    return;
  }

  child_process_ = child_process.Pass();
}

// Callback for |mojo::embedder::ConnectToSlave()|.
void ChildProcessHost::DidConnectToSlave() {
  DVLOG(2) << "ChildProcessHost::DidConnectToSlave()";
}

base::Process ChildProcessHost::DoLaunch(scoped_ptr<LaunchData> launch_data) {
  static const char* kForwardSwitches[] = {
      switches::kTraceToConsole, switches::kV, switches::kVModule,
  };

  base::CommandLine child_command_line(launch_data->child_path);
  child_command_line.CopySwitchesFrom(*base::CommandLine::ForCurrentProcess(),
                                      kForwardSwitches,
                                      arraysize(kForwardSwitches));
  child_command_line.AppendSwitchASCII(switches::kChildConnectionId,
                                       launch_data->child_connection_id);

  base::FileHandleMappingVector fds_to_remap;
  fds_to_remap.push_back(
      std::pair<int, int>(launch_data->platform_pipe.handle1.get().fd,
                          base::GlobalDescriptors::kBaseDescriptor));
  base::LaunchOptions options;
  options.fds_to_remap = &fds_to_remap;
#if defined(OS_LINUX)
  options.allow_new_privs = launch_data->options.allow_new_privs;
#endif
  DVLOG(2) << "Launching child with command line: "
           << child_command_line.GetCommandLineString();
  base::Process child_process =
      base::LaunchProcess(child_command_line, options);
  if (child_process.IsValid())
    launch_data->platform_pipe.handle1.reset();
  return child_process.Pass();
}

void ChildProcessHost::AppCompleted(int32_t result) {
  if (!on_app_complete_.is_null()) {
    auto on_app_complete = on_app_complete_;
    on_app_complete_.reset();
    on_app_complete.Run(result);
  }
}

void ChildProcessHost::OnConnectionError() {
  AppCompleted(MOJO_RESULT_UNKNOWN);
}

}  // namespace shell
