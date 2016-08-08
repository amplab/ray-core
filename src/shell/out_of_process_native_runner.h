// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_OUT_OF_PROCESS_NATIVE_RUNNER_H_
#define SHELL_OUT_OF_PROCESS_NATIVE_RUNNER_H_

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "shell/application_manager/native_application_options.h"
#include "shell/application_manager/native_runner.h"

namespace shell {

class Context;
class ChildProcessHost;

// An implementation of |NativeRunner| that loads/runs the given app (from the
// file system) in a separate process (of its own).
class OutOfProcessNativeRunner : public NativeRunner {
 public:
  OutOfProcessNativeRunner(Context* context,
                           const NativeApplicationOptions& options);
  ~OutOfProcessNativeRunner() override;

  // |NativeRunner| method:
  void Start(const base::FilePath& app_path,
             mojo::InterfaceRequest<mojo::Application> application_request,
             const base::Closure& app_completed_callback) override;

 private:
  // |ChildProcessHost::StartApp()| callback:
  void AppCompleted(int32_t result);

  Context* const context_;
  NativeApplicationOptions const options_;

  base::FilePath app_path_;
  base::Closure app_completed_callback_;

  scoped_ptr<ChildProcessHost> child_process_host_;

  bool connect_to_running_process_;

  DISALLOW_COPY_AND_ASSIGN(OutOfProcessNativeRunner);
};

class OutOfProcessNativeRunnerFactory : public NativeRunnerFactory {
 public:
  explicit OutOfProcessNativeRunnerFactory(Context* context)
      : context_(context) {}
  ~OutOfProcessNativeRunnerFactory() override {}

  scoped_ptr<NativeRunner> Create(
      const NativeApplicationOptions& options) override;

 private:
  Context* const context_;

  DISALLOW_COPY_AND_ASSIGN(OutOfProcessNativeRunnerFactory);
};

}  // namespace shell

#endif  // SHELL_OUT_OF_PROCESS_NATIVE_RUNNER_H_
