// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_support/process_controller_impl.h"

#include <errno.h>
#include <signal.h>
#include <sys/types.h>

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/task_runner.h"
#include "base/threading/simple_thread.h"
#include "mojo/services/files/interfaces/types.mojom.h"
#include "services/native_support/process_io_redirection.h"

namespace native_support {

namespace {

void WaitForProcess(
    base::Process process,
    scoped_refptr<base::TaskRunner> done_runner,
    const base::Callback<void(mojo::files::Error, int32_t)>& done_callback) {
  int exit_status = 0;
  mojo::files::Error result = process.WaitForExit(&exit_status)
                                  ? mojo::files::Error::OK
                                  : mojo::files::Error::UNKNOWN;
  done_runner->PostTask(
      FROM_HERE,
      base::Bind(done_callback, result, static_cast<int32_t>(exit_status)));
}

void TerminateProcess(base::Process process) {
  if (!process.Terminate(-1, true))
    LOG(ERROR) << "Failed to kill PID " << process.Pid();
}

}  // namespace

ProcessControllerImpl::ProcessControllerImpl(
    scoped_refptr<base::TaskRunner> worker_runner,
    mojo::InterfaceRequest<ProcessController> request,
    base::Process process,
    std::unique_ptr<ProcessIORedirection> process_io_redirection)
    : worker_runner_(worker_runner.Pass()),
      binding_(this, request.Pass()),
      process_(process.Pass()),
      process_io_redirection_(std::move(process_io_redirection)),
      weak_factory_(this) {
  DCHECK(process_.IsValid());
}

ProcessControllerImpl::~ProcessControllerImpl() {
  if (process_.IsValid()) {
    worker_runner_->PostTask(
        FROM_HERE, base::Bind(&TerminateProcess, base::Passed(&process_)));
  }
}

void ProcessControllerImpl::Wait(const WaitCallback& callback) {
  if (!process_.IsValid()) {
    // TODO(vtl): This isn't quite right.
    callback.Run(mojo::files::Error::UNAVAILABLE, 0);
    return;
  }

  worker_runner_->PostTask(
      FROM_HERE, base::Bind(&WaitForProcess, base::Passed(&process_),
                            base::MessageLoop::current()->task_runner(),
                            base::Bind(&ProcessControllerImpl::OnWaitComplete,
                                       weak_factory_.GetWeakPtr(), callback)));
}

void ProcessControllerImpl::Kill(int32_t signal, const KillCallback& callback) {
  callback.Run(KillHelper(signal));
}

void ProcessControllerImpl::OnWaitComplete(const WaitCallback& callback,
                                           mojo::files::Error result,
                                           int32_t exit_status) {
  callback.Run(result, exit_status);
}

mojo::files::Error ProcessControllerImpl::KillHelper(int32_t signal) {
  if (signal < 0)
    return mojo::files::Error::INVALID_ARGUMENT;

  if (!process_.IsValid()) {
    LOG(ERROR) << "Kill() called after Wait()";
    // TODO(vtl): This error code isn't quite right, but "unavailable" (which
    // would also be wrong) is used for a more appropriate purpose below.
    return mojo::files::Error::INVALID_ARGUMENT;
  }

  // |base::HandleType| is just a typedef for |pid_t|.
  pid_t pid = process_.Handle();

  // Note: |kill()| is not interruptible.
  if (kill(pid, static_cast<int>(signal)) == 0)
    return mojo::files::Error::OK;

  switch (errno) {
    case EINVAL:
      return mojo::files::Error::INVALID_ARGUMENT;
    case EPERM:
      return mojo::files::Error::PERMISSION_DENIED;
    case ESRCH:
      return mojo::files::Error::UNAVAILABLE;
    default:
      break;
  }
  return mojo::files::Error::UNKNOWN;
}

}  // namespace native_support
