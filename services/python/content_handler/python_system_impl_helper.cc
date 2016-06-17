// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "python_system_impl_helper.h"

#include <Python.h>

#include "base/bind.h"
#include "base/location.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/public/python/src/common.h"

namespace {
class QuitCurrentRunLoop : public mojo::Closure::Runnable {
 public:
  void Run() const override {
    base::MessageLoop::current()->Quit();
  }

  static mojo::Closure::Runnable* NewInstance() {
    return new QuitCurrentRunLoop();
  }
};

}  // namespace
namespace services {
namespace python {
namespace content_handler {

PythonRunLoop::PythonRunLoop()
    : loop_(mojo::common::MessagePumpMojo::Create()) {
}

PythonRunLoop::~PythonRunLoop() {
}

void PythonRunLoop::Run() {
  loop_.Run();
}

void PythonRunLoop::RunUntilIdle() {
  loop_.RunUntilIdle();
}

void PythonRunLoop::Quit() {
  loop_.Quit();
}

void PythonRunLoop::PostDelayedTask(PyObject* callable, MojoTimeTicks delay) {
  mojo::Closure::Runnable* quit_runnable =
      mojo::python::NewRunnableFromCallable(callable, loop_.QuitClosure());

  loop_.PostDelayedTask(
      FROM_HERE, base::Bind(&mojo::Closure::Run,
                            base::Owned(new mojo::Closure(quit_runnable))),
      base::TimeDelta::FromMicroseconds(delay));
}

mojo::python::PythonAsyncWaiter* NewAsyncWaiter() {
  return new mojo::python::PythonAsyncWaiter(
      mojo::Closure(QuitCurrentRunLoop::NewInstance()));
}

}  // namespace content_handler
}  // namespace python
}  // namespace services
