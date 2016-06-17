// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/task_runners.h"

#include "base/threading/sequenced_worker_pool.h"
#include "mojo/edk/base_edk/platform_handle_watcher_impl.h"
#include "mojo/edk/base_edk/platform_task_runner_impl.h"
#include "mojo/edk/util/make_unique.h"

using mojo::util::MakeRefCounted;
using mojo::util::MakeUnique;

namespace shell {

namespace {

const size_t kMaxBlockingPoolThreads = 3;

scoped_ptr<base::Thread> CreateIOThread(const char* name) {
  scoped_ptr<base::Thread> thread(new base::Thread(name));
  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  thread->StartWithOptions(options);
  return thread;
}

}  // namespace

TaskRunners::TaskRunners(
    const scoped_refptr<base::SingleThreadTaskRunner>& shell_runner)
    : shell_runner_(
          MakeRefCounted<base_edk::PlatformTaskRunnerImpl>(shell_runner)),
      io_thread_(CreateIOThread("io_thread")),
      io_runner_(MakeRefCounted<base_edk::PlatformTaskRunnerImpl>(
          io_thread_->task_runner())),
      io_watcher_(MakeUnique<base_edk::PlatformHandleWatcherImpl>(
          static_cast<base::MessageLoopForIO*>(io_thread_->message_loop()))),
      blocking_pool_(new base::SequencedWorkerPool(kMaxBlockingPoolThreads,
                                                   "blocking_pool")) {}

TaskRunners::~TaskRunners() {
  blocking_pool_->Shutdown();
}

}  // namespace shell
