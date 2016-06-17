// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PYTHON_PYTHON_SYSTEM_IMPL_HELPER_H_
#define SERVICES_PYTHON_PYTHON_SYSTEM_IMPL_HELPER_H_

#include <Python.h>

#include "base/message_loop/message_loop.h"
#include "mojo/public/python/src/common.h"

namespace services {
namespace python {
namespace content_handler {

// Run loop for python
class PythonRunLoop {
 public:
  PythonRunLoop();

  ~PythonRunLoop();

  void Run();

  void RunUntilIdle();

  void Quit();

  // Post a task to be executed roughtly after delay microseconds
  void PostDelayedTask(PyObject* callable, int64 delay);
 private:
  base::MessageLoop loop_;
};

mojo::python::PythonAsyncWaiter* NewAsyncWaiter();

}  // namespace content_handler
}  // namespace python
}  // namespace services

#endif  // SERVICES_PYTHON_PYTHON_SYSTEM_IMPL_HELPER_H_

