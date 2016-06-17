// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_APPLICATION_MANAGER_NATIVE_RUNNER_H_
#define SHELL_APPLICATION_MANAGER_NATIVE_RUNNER_H_

#include "base/callback_forward.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "shell/native_application_support.h"

namespace base {
class FilePath;
}

namespace shell {

struct NativeApplicationOptions;

// ApplicationManager requires implementations of NativeRunner and
// NativeRunnerFactory to run native applications.
class NativeRunner {
 public:
  virtual ~NativeRunner() {}

  // Loads the app in the file at |app_path| and runs it on some other
  // thread/process.
  // |app_completed_callback| is posted (to the thread on which |Start()| was
  // called) after |MojoMain()| completes, or on any error (including if it
  // fails to start).
  // TODO(vtl): |app_path| should probably be moved to the factory's Create().
  // Rationale: The factory may need information from the file to decide what
  // kind of NativeRunner to make.
  virtual void Start(
      const base::FilePath& app_path,
      mojo::InterfaceRequest<mojo::Application> application_request,
      const base::Closure& app_completed_callback) = 0;
};

class NativeRunnerFactory {
 public:
  virtual ~NativeRunnerFactory() {}
  virtual scoped_ptr<NativeRunner> Create(
      const NativeApplicationOptions& options) = 0;
};

}  // namespace shell

#endif  // SHELL_APPLICATION_MANAGER_NATIVE_RUNNER_H_
