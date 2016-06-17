// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_ENVIRONMENT_ENVIRONMENT_H_
#define MOJO_PUBLIC_CPP_ENVIRONMENT_ENVIRONMENT_H_

#include "mojo/public/cpp/system/macros.h"

struct MojoAsyncWaiter;
struct MojoLogger;

namespace mojo {

// This class just acts as a "namespace": it only has static methods (whose
// implementation may be varied). Note that some implementations may require
// their own explicit initialization/shut down functions to be called.
class Environment {
 public:
  static const MojoAsyncWaiter* GetDefaultAsyncWaiter();
  // Setting the default async waiter to null will use the original default
  // implementation.
  static void SetDefaultAsyncWaiter(const MojoAsyncWaiter* async_waiter);

  static const MojoLogger* GetDefaultLogger();
  // Setting the logger to null will use the will use the original default
  // implementation.
  static void SetDefaultLogger(const MojoLogger* logger);

  // These instantiate and destroy an environment-specific run loop for the
  // current thread, allowing |GetDefaultAsyncWaiter()| to be used. (The run
  // loop itself should be accessible via thread-local storage, using methods
  // specific to the run loop implementation.) Creating and destroying nested
  // run loops is not supported.
  static void InstantiateDefaultRunLoop();
  static void DestroyDefaultRunLoop();

 private:
  Environment() = delete;
  ~Environment() = delete;
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_ENVIRONMENT_ENVIRONMENT_H_
