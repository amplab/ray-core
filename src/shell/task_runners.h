// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_TASK_RUNNERS_H_
#define SHELL_TASK_RUNNERS_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "mojo/edk/platform/platform_handle_watcher.h"
#include "mojo/edk/platform/task_runner.h"
#include "mojo/edk/util/ref_ptr.h"

namespace base {
class SequencedWorkerPool;
}

namespace shell {

// A context object that contains the common task runners for the shell's main
// process.
class TaskRunners {
 public:
  explicit TaskRunners(
      const scoped_refptr<base::SingleThreadTaskRunner>& shell_runner);
  ~TaskRunners();

  const mojo::util::RefPtr<mojo::platform::TaskRunner>& shell_runner() const {
    return shell_runner_;
  }

  const mojo::util::RefPtr<mojo::platform::TaskRunner>& io_runner() const {
    return io_runner_;
  }

  mojo::platform::PlatformHandleWatcher* io_watcher() const {
    return io_watcher_.get();
  }

  base::SequencedWorkerPool* blocking_pool() const {
    return blocking_pool_.get();
  }

 private:
  mojo::util::RefPtr<mojo::platform::TaskRunner> shell_runner_;
  scoped_ptr<base::Thread> io_thread_;
  mojo::util::RefPtr<mojo::platform::TaskRunner> io_runner_;
  std::unique_ptr<mojo::platform::PlatformHandleWatcher> io_watcher_;

  scoped_refptr<base::SequencedWorkerPool> blocking_pool_;

  DISALLOW_COPY_AND_ASSIGN(TaskRunners);
};

}  // namespace shell

#endif  // SHELL_TASK_RUNNERS_H_
