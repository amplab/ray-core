// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/environment/environment.h"

#include <assert.h>

#include "mojo/public/c/environment/logger.h"
#include "mojo/public/cpp/environment/lib/default_async_waiter.h"
#include "mojo/public/cpp/environment/lib/default_logger.h"
#include "mojo/public/cpp/utility/run_loop.h"

namespace mojo {

const MojoAsyncWaiter* g_default_async_waiter = &internal::kDefaultAsyncWaiter;
const MojoLogger* g_default_logger = &internal::kDefaultLogger;

// static
const MojoAsyncWaiter* Environment::GetDefaultAsyncWaiter() {
  return g_default_async_waiter;
}

// static
void Environment::SetDefaultAsyncWaiter(const MojoAsyncWaiter* async_waiter) {
  g_default_async_waiter =
      async_waiter ? async_waiter : &internal::kDefaultAsyncWaiter;
}

// static
const MojoLogger* Environment::GetDefaultLogger() {
  return g_default_logger;
}

// static
void Environment::SetDefaultLogger(const MojoLogger* logger) {
  g_default_logger = logger ? logger : &internal::kDefaultLogger;
}

// static
void Environment::InstantiateDefaultRunLoop() {
  assert(!RunLoop::current());
  // Not leaked: accessible from |RunLoop::current()|.
  RunLoop* run_loop = new RunLoop();
  MOJO_ALLOW_UNUSED_LOCAL(run_loop);
  assert(run_loop == RunLoop::current());
}

// static
void Environment::DestroyDefaultRunLoop() {
  assert(RunLoop::current());
  delete RunLoop::current();
  assert(!RunLoop::current());
}

}  // namespace mojo
