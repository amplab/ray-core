#include <Python.h>

#include "base/at_exit.h"
// #include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
// #include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/posix/global_descriptors.h"
#include "base/single_thread_task_runner.h"
// #include "base/synchronization/waitable_event.h"
// #include "base/thread_task_runner_handle.h"
// #include "base/threading/thread.h"
// #include "base/threading/thread_checker.h"
// #include "mojo/edk/base_edk/platform_handle_watcher_impl.h"
// #include "mojo/edk/base_edk/platform_task_runner_impl.h"
// #include "mojo/edk/embedder/embedder.h"
// #include "mojo/edk/embedder/multiprocess_embedder.h"
// #include "mojo/edk/embedder/simple_platform_support.h"
// #include "mojo/edk/embedder/slave_process_delegate.h"
#include "mojo/edk/platform/platform_handle.h"
#include "mojo/edk/platform/platform_handle_watcher.h"
#include "mojo/edk/platform/platform_pipe.h"
// #include "mojo/edk/platform/scoped_platform_handle.h"
#include "mojo/edk/platform/task_runner.h"
// #include "mojo/edk/util/make_unique.h"
// #include "mojo/edk/util/ref_ptr.h"
// #include "mojo/message_pump/message_pump_mojo.h"
// #include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/message_pipe.h"
// #include "shell/child_controller.mojom.h"
// #include "shell/child_switches.h"
#include "shell/init.h"
#include "shell/native_application_support.h"

#include <iostream>
#include <chrono>
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "exchange_file_descriptor.h"

#include "app_context.h"
#include "blocker.h"
#include "child_controller_impl.h"

void start_main_loop(const std::string& address, const std::string& child_connection_id) {
  FileDescriptorReceiver receiver(address);
  int fd = receiver.Receive(); // file descriptor to bootstrap the IPC from

  mojo::platform::ScopedPlatformHandle platform_handle((mojo::platform::PlatformHandle(fd)));
  shell::AppContext app_context;
  app_context.Init(platform_handle.Pass());

  shell::Blocker blocker;
  // TODO(vtl): With C++14 lambda captures, this can be made nicer.
  const shell::Blocker::Unblocker unblocker = blocker.GetUnblocker();
  app_context.controller_runner()->PostTask(
      [&app_context, &child_connection_id, &unblocker]() {
        shell::ChildControllerImpl::Init(&app_context, child_connection_id,
                                         unblocker);
      });
  // This will block, then run whatever the controller wants.
  blocker.Block();

  app_context.Shutdown();
}

extern "C" {

PyObject* connect(PyObject* self, PyObject* args) {
  const char* address;
  const char* child_connection_id;
  if (!PyArg_ParseTuple(args, "ss", &address, &child_connection_id)) {
    return NULL;
  }
  start_main_loop(std::string(address), std::string(child_connection_id));
  Py_RETURN_NONE;
}

static PyMethodDef RayLibMethods[] = {
  { "connect", connect, METH_VARARGS, "connect to the object store" },
  { NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC initlibraylib(void) {
  base::AtExitManager at_exit;
  int argc = 1;
  const char* argv[] = { "rayclient", NULL };
  base::CommandLine::Init(argc, argv);
  shell::InitializeLogging();
  Py_InitModule3("libraylib", RayLibMethods, "ray-core python client library");
}

}
