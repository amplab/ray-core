// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_SUPPORT_PROCESS_IMPL_H_
#define SERVICES_NATIVE_SUPPORT_PROCESS_IMPL_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/task_runner.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/native_support/interfaces/process.mojom.h"

namespace mojo {
struct ConnectionContext;
}

namespace native_support {

class ProcessIORedirection;

class ProcessImpl : public Process {
 public:
  ProcessImpl(scoped_refptr<base::TaskRunner> worker_runner,
              const mojo::ConnectionContext& connection_context,
              mojo::InterfaceRequest<Process> request);
  ~ProcessImpl() override;

  // |Process| implementation:
  void Spawn(mojo::Array<uint8_t> path,
             mojo::Array<mojo::Array<uint8_t>> argv,
             mojo::Array<mojo::Array<uint8_t>> envp,
             mojo::InterfaceHandle<mojo::files::File> stdin_file,
             mojo::InterfaceHandle<mojo::files::File> stdout_file,
             mojo::InterfaceHandle<mojo::files::File> stderr_file,
             mojo::InterfaceRequest<ProcessController> process_controller,
             const SpawnCallback& callback) override;
  void SpawnWithTerminal(
      mojo::Array<uint8_t> path,
      mojo::Array<mojo::Array<uint8_t>> argv,
      mojo::Array<mojo::Array<uint8_t>> envp,
      mojo::InterfaceHandle<mojo::files::File> terminal_file,
      mojo::InterfaceRequest<ProcessController> process_controller,
      const SpawnWithTerminalCallback& callback) override;

 private:
  // Note: We take advantage of the fact that |SpawnCallback| and
  // |SpawnWithTerminalCallback| are the same.
  void SpawnImpl(mojo::Array<uint8_t> path,
                 mojo::Array<mojo::Array<uint8_t>> argv,
                 mojo::Array<mojo::Array<uint8_t>> envp,
                 std::unique_ptr<ProcessIORedirection> process_io_redirection,
                 const std::vector<int>& fds_to_inherit,
                 mojo::InterfaceRequest<ProcessController> process_controller,
                 const SpawnCallback& callback);

  scoped_refptr<base::TaskRunner> worker_runner_;
  mojo::StrongBinding<Process> binding_;

  DISALLOW_COPY_AND_ASSIGN(ProcessImpl);
};

}  // namespace native_support

#endif  // SERVICES_NATIVE_SUPPORT_PROCESS_IMPL_H_
