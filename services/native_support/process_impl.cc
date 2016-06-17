// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_support/process_impl.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "build/build_config.h"
#include "mojo/services/files/interfaces/types.mojom.h"
#include "services/native_support/make_pty_pair.h"
#include "services/native_support/process_controller_impl.h"
#include "services/native_support/process_io_redirection.h"

using mojo::files::FilePtr;

namespace native_support {

namespace {

class SetsidPreExecDelegate : public base::LaunchOptions::PreExecDelegate {
 public:
  SetsidPreExecDelegate() {}
  ~SetsidPreExecDelegate() override {}

  void RunAsyncSafe() override {
    static const char kErrorMessage[] = "setsid() failed";

    // Note: |setsid()| and |write()| are both async-signal-safe.
    if (setsid() == static_cast<pid_t>(-1))
      write(STDERR_FILENO, kErrorMessage, sizeof(kErrorMessage) - 1);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SetsidPreExecDelegate);
};

}  // namespace

// TODO(vtl): This should do something with the |connection_context|.
ProcessImpl::ProcessImpl(scoped_refptr<base::TaskRunner> worker_runner,
                         const mojo::ConnectionContext& connection_context,
                         mojo::InterfaceRequest<Process> request)
    : worker_runner_(worker_runner.Pass()), binding_(this, request.Pass()) {}

ProcessImpl::~ProcessImpl() {}

void ProcessImpl::Spawn(
    mojo::Array<uint8_t> path,
    mojo::Array<mojo::Array<uint8_t>> argv,
    mojo::Array<mojo::Array<uint8_t>> envp,
    mojo::InterfaceHandle<mojo::files::File> stdin_file,
    mojo::InterfaceHandle<mojo::files::File> stdout_file,
    mojo::InterfaceHandle<mojo::files::File> stderr_file,
    mojo::InterfaceRequest<ProcessController> process_controller,
    const SpawnCallback& callback) {
  std::vector<int> fds_to_inherit(3, -1);

  // stdin:
  base::ScopedFD stdin_fd;
  base::ScopedFD stdin_parent_fd;
  if (stdin_file) {
    int stdin_pipe_fds[2] = {-1, -1};
    CHECK_EQ(pipe(stdin_pipe_fds), 0);
    stdin_fd.reset(stdin_pipe_fds[0]);
    stdin_parent_fd.reset(stdin_pipe_fds[1]);
  } else {
    stdin_fd.reset(HANDLE_EINTR(open("/dev/null", O_RDONLY)));
  }
  fds_to_inherit[STDIN_FILENO] = stdin_fd.get();

  // stdout:
  base::ScopedFD stdout_fd;
  base::ScopedFD stdout_parent_fd;
  if (stdout_file) {
    int stdout_pipe_fds[2] = {-1, -1};
    CHECK_EQ(pipe(stdout_pipe_fds), 0);
    stdout_fd.reset(stdout_pipe_fds[1]);
    stdout_parent_fd.reset(stdout_pipe_fds[0]);
  } else {
    stdout_fd.reset(HANDLE_EINTR(open("/dev/null", O_WRONLY)));
  }
  fds_to_inherit[STDOUT_FILENO] = stdout_fd.get();

  // stderr:
  base::ScopedFD stderr_fd;
  base::ScopedFD stderr_parent_fd;
  if (stderr_file) {
    int stderr_pipe_fds[2] = {-1, -1};
    CHECK_EQ(pipe(stderr_pipe_fds), 0);
    stderr_fd.reset(stderr_pipe_fds[1]);
    stderr_parent_fd.reset(stderr_pipe_fds[0]);
  } else {
    stderr_fd.reset(HANDLE_EINTR(open("/dev/null", O_WRONLY)));
  }
  fds_to_inherit[STDERR_FILENO] = stderr_fd.get();

  std::unique_ptr<ProcessIORedirection> process_io_redirection(
      new ProcessIORedirectionForStdIO(
          FilePtr::Create(std::move(stdin_file)),
          FilePtr::Create(std::move(stdout_file)),
          FilePtr::Create(std::move(stderr_file)), stdin_parent_fd.Pass(),
          stdout_parent_fd.Pass(), stderr_parent_fd.Pass()));

  SpawnImpl(path.Pass(), argv.Pass(), envp.Pass(),
            std::move(process_io_redirection), fds_to_inherit,
            process_controller.Pass(), callback);
}

void ProcessImpl::SpawnWithTerminal(
    mojo::Array<uint8_t> path,
    mojo::Array<mojo::Array<uint8_t>> argv,
    mojo::Array<mojo::Array<uint8_t>> envp,
    mojo::InterfaceHandle<mojo::files::File> terminal_file,
    mojo::InterfaceRequest<ProcessController> process_controller,
    const SpawnWithTerminalCallback& callback) {
  DCHECK(terminal_file);

  std::vector<int> fds_to_inherit(3, -1);

  base::ScopedFD master_fd;
  base::ScopedFD slave_fd;
  int errno_value = 0;
  if (!MakePtyPair(&master_fd, &slave_fd, &errno_value)) {
    // TODO(vtl): Well, this is dumb (we should use errno_value).
    callback.Run(mojo::files::Error::UNKNOWN);
    return;
  }

  // stdin:
  base::ScopedFD stdin_fd(slave_fd.Pass());
  fds_to_inherit[STDIN_FILENO] = stdin_fd.get();

  // stdout:
  base::ScopedFD stdout_fd(HANDLE_EINTR(dup(stdin_fd.get())));
  fds_to_inherit[STDOUT_FILENO] = stdout_fd.get();

  // stderr:
  base::ScopedFD stderr_fd(HANDLE_EINTR(dup(stdin_fd.get())));
  fds_to_inherit[STDERR_FILENO] = stderr_fd.get();

  std::unique_ptr<ProcessIORedirection> process_io_redirection(
      new ProcessIORedirectionForTerminal(
          FilePtr::Create(std::move(terminal_file)), master_fd.Pass()));

  SpawnImpl(path.Pass(), argv.Pass(), envp.Pass(),
            std::move(process_io_redirection), fds_to_inherit,
            process_controller.Pass(), callback);
}

void ProcessImpl::SpawnImpl(
    mojo::Array<uint8_t> path,
    mojo::Array<mojo::Array<uint8_t>> argv,
    mojo::Array<mojo::Array<uint8_t>> envp,
    std::unique_ptr<ProcessIORedirection> process_io_redirection,
    const std::vector<int>& fds_to_inherit,
    mojo::InterfaceRequest<ProcessController> process_controller,
    const SpawnCallback& callback) {
  DCHECK(!path.is_null());
  DCHECK(process_controller.is_pending());

  // Add terminating null character to the "strings", so we can just use
  // |.data()|.
  path.push_back(0u);

  size_t argc = std::max(argv.size(), static_cast<size_t>(1));
  std::vector<const char*> argv_ptrs(argc);
  if (argv.is_null()) {
    argv_ptrs[0] = reinterpret_cast<const char*>(path.data());
  } else {
    if (!argv.size() ||
        argv.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
      callback.Run(mojo::files::Error::INVALID_ARGUMENT);
      return;
    }
    // TODO(vtl): Currently, we don't support setting argv[0], due to
    // |base::CommandLine| limitations.
    argv_ptrs[0] = reinterpret_cast<const char*>(path.data());
    for (size_t i = 1; i < argv.size(); i++) {
      DCHECK(!argv[i].is_null());
      // Add terminating null character.
      argv[i].push_back(0u);
      argv_ptrs[i] = reinterpret_cast<const char*>(argv[i].data());
    }
  }
  base::CommandLine command_line(static_cast<int>(argc), argv_ptrs.data());

  bool inherit_environment = true;
  base::EnvironmentMap environment_map;
  if (!envp.is_null()) {
    inherit_environment = false;
    for (size_t i = 0; i < envp.size(); i++) {
      DCHECK(!envp[i].is_null());
      // Add terminating null character.
      // Note: We prefer to do this, rather than use |envp[i].size()| to make
      // |s| below, since we want to truncate at the first null character (in
      // case |envp[i]| has an embedded null).
      envp[i].push_back(0u);
      std::string s(reinterpret_cast<const char*>(envp[i].data()));
      size_t equals_pos = s.find_first_of('=');
      environment_map[s.substr(0, equals_pos)] =
          (equals_pos == std::string::npos) ? std::string()
                                            : s.substr(equals_pos + 1);
    }
  }

  base::FileHandleMappingVector fd_mapping;
  DCHECK(fds_to_inherit.size() >= 3);
  for (size_t i = 0; i < fds_to_inherit.size(); i++) {
    DCHECK_GE(fds_to_inherit[i], 0);
    fd_mapping.push_back(
        std::make_pair(fds_to_inherit[i], static_cast<int>(i)));
  }

  SetsidPreExecDelegate pre_exec_delegate;
  base::LaunchOptions launch_options;
  launch_options.wait = false;
  launch_options.environ.swap(environment_map);
  launch_options.clear_environ = !inherit_environment;
  launch_options.fds_to_remap = &fd_mapping;
  // launch_options.maximize_rlimits
  launch_options.new_process_group = false;
  // launch_options.clone_flags = 0;
#if defined(OS_LINUX)
  launch_options.allow_new_privs = true;
#endif
  // launch_options.kill_on_parent_death = true;
  // launch_options.current_directory
  launch_options.pre_exec_delegate = &pre_exec_delegate;

  base::Process process = LaunchProcess(command_line, launch_options);
  // Note: Failure is extremely unusual. E.g., it won't fail even if |path|
  // doesn't exist (since fork succeeds; it's the exec that fails).
  if (!process.IsValid()) {
    // TODO(vtl): Well, this is dumb (can we check errno?).
    callback.Run(mojo::files::Error::UNKNOWN);
    return;
  }

  new ProcessControllerImpl(worker_runner_, process_controller.Pass(),
                            process.Pass(), std::move(process_io_redirection));
  callback.Run(mojo::files::Error::OK);
}

}  // namespace native_support
