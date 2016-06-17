# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# distutils language = c++

cimport c_async_waiter
cimport c_base
cimport c_export

from libc.stdint cimport uintptr_t


cdef class RunLoop(object):
  """RunLoop to use when using asynchronous operations on handles."""

  cdef c_base.PythonRunLoop* c_run_loop

  def __init__(self):
    self.c_run_loop = new c_base.PythonRunLoop()

  def __dealloc__(self):
    del self.c_run_loop

  def Run(self):
    """Run the runloop until Quit is called."""
    self.c_run_loop.Run()

  def RunUntilIdle(self):
    """Run the runloop until Quit is called or no operation is waiting."""
    self.c_run_loop.RunUntilIdle()

  def Quit(self):
    """Quit the runloop."""
    self.c_run_loop.Quit()

  def PostDelayedTask(self, runnable, delay=0):
    """
    Post a task on the runloop. This must be called from the thread owning the
    runloop.
    """
    self.c_run_loop.PostDelayedTask(runnable, delay)


# We use a wrapping class to be able to call the C++ class PythonAsyncWaiter
# across module boundaries.
cdef class AsyncWaiter(object):
  cdef c_async_waiter.PythonAsyncWaiter* _c_async_waiter

  def __init__(self):
    self._c_async_waiter = c_base.NewAsyncWaiter()

  def __dealloc__(self):
    del self._c_async_waiter

  def AsyncWait(self, handle, signals, deadline, callback):
    return self._c_async_waiter.AsyncWait(handle, signals, deadline, callback)

  def CancelWait(self, wait_id):
    self._c_async_waiter.CancelWait(wait_id)
