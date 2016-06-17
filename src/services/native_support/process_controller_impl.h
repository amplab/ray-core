// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_SUPPORT_PROCESS_CONTROLLER_IMPL_H_
#define SERVICES_NATIVE_SUPPORT_PROCESS_CONTROLLER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/process/process.h"
#include "base/task_runner.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/files/interfaces/file.mojom.h"
#include "mojo/services/files/interfaces/types.mojom.h"
#include "mojo/services/native_support/interfaces/process.mojom.h"

namespace native_support {

class ProcessIORedirection;

class ProcessControllerImpl : public ProcessController {
 public:
  ProcessControllerImpl(
      scoped_refptr<base::TaskRunner> worker_runner,
      mojo::InterfaceRequest<ProcessController> request,
      base::Process process,
      std::unique_ptr<ProcessIORedirection> process_io_redirection);
  ~ProcessControllerImpl() override;

  // |ProcessController| implementation:
  void Wait(const WaitCallback& callback) override;
  void Kill(int32_t signal, const KillCallback& callback) override;

 private:
  void OnWaitComplete(const WaitCallback& callback,
                      mojo::files::Error result,
                      int32_t exit_status);

  mojo::files::Error KillHelper(int32_t signal);

  scoped_refptr<base::TaskRunner> worker_runner_;
  mojo::StrongBinding<ProcessController> binding_;
  base::Process process_;
  std::unique_ptr<ProcessIORedirection> process_io_redirection_;

  base::WeakPtrFactory<ProcessControllerImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProcessControllerImpl);
};

}  // namespace native_support

#endif  // SERVICES_NATIVE_SUPPORT_PROCESS_CONTROLLER_IMPL_H_
