// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_CHILD_PROCESS_HOST_H_
#define SHELL_CHILD_PROCESS_HOST_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/process/process.h"
#include "mojo/edk/embedder/channel_info_forward.h"
#include "shell/child_controller.mojom.h"

namespace shell {

class Context;
struct NativeApplicationOptions;

// Child process host: parent-process representation of a child process, which
// hosts/runs a native Mojo application loaded from the file system. This class
// handles launching and communicating with the child process.
//
// This class is not thread-safe. It should be created/used/destroyed on a
// single thread.
class ChildProcessHost {
 public:
  explicit ChildProcessHost(Context* context);
  // TODO(vtl): Virtual because |DidStart()| is, even though it shouldn't be
  // (see |DidStart()|).
  virtual ~ChildProcessHost();

  // |Start()|s the child process; calls |DidStart()| (on the thread on which
  // |Start()| was called) when the child has been started (or failed to start).
  // After calling |Start()|, this object must not be destroyed until
  // |DidStart()| has been called.
  // TODO(vtl): Consider using weak pointers and removing this requirement.
  // TODO(vtl): This should probably take a callback instead.
  // TODO(vtl): Consider merging this with |StartApp()|.
  void Start(const NativeApplicationOptions& options,
             bool connect_to_running_process = false);

  // Waits for the child process to terminate, and returns its exit code.
  // Note: If |Start()| has been called, this must not be called until the
  // callback has been called.
  int Join();

  // Methods relayed to the |ChildController|. These methods may be only be
  // called after |Start()|, but may be called immediately (without waiting for
  // |DidStart()|).

  // Like |ChildController::StartApp()|, but with one difference:
  // |on_app_complete| will *always* get called, even on connection error (or
  // even if the child process failed to start at all).
  void StartApp(const mojo::String& app_path,
                mojo::InterfaceRequest<mojo::Application> application_request,
                const ChildController::StartAppCallback& on_app_complete);
  void ExitNow(int32_t exit_code);

  // TODO(vtl): This is virtual, so tests can override it, but really |Start()|
  // should take a callback (see above) and this should be private.
  virtual void DidStart(base::Process child_process);

 private:
  struct LaunchData;

  // Callback for |mojo::embedder::ConnectToSlave()|.
  void DidConnectToSlave();

  // Note: This is probably executed on a different thread (namely, using the
  // blocking pool).
  base::Process DoLaunch(scoped_ptr<LaunchData> launch_data);

  void AppCompleted(int32_t result);
  void OnConnectionError();

  Context* const context_;

  ChildControllerPtr controller_;
  mojo::embedder::ChannelInfo* channel_info_;
  ChildController::StartAppCallback on_app_complete_;

  base::Process child_process_;

  DISALLOW_COPY_AND_ASSIGN(ChildProcessHost);
};

}  // namespace shell

#endif  // SHELL_CHILD_PROCESS_HOST_H_
