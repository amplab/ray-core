#include <iostream>

#include <string>
#include <thread>
#include <Python.h>
#include "base/at_exit.h"
#include "base/command_line.h"
#include "mojo/edk/util/make_unique.h"
#include "shell/init.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "examples/echo/echo.mojom.h"

#include "client.cc"

extern "C" {

struct ray_client_state {
  std::unique_ptr<base::AtExitManager> at_exit;
  std::thread connector_app_thread;
  mojo::InterfaceHandle<mojo::examples::Echo> echo_handle;
};

static ray_client_state global_state;

static PyObject* connect(PyObject* self, PyObject* args) {
  const char* address;
  const char* child_connection_id;
  if (!PyArg_ParseTuple(args, "ss", &address, &child_connection_id)) {
    return NULL;
  }
  global_state.at_exit = mojo::util::MakeUnique<base::AtExitManager>();
  global_state.connector_app_thread =
    start_rayclient<mojo::examples::Echo>(std::string(address),
      std::string(child_connection_id),
      &global_state.echo_handle);
  auto echo = mojo::SynchronousInterfacePtr<mojo::examples::Echo>::Create(global_state.echo_handle.Pass());
  mojo::String result;
  echo->EchoString("hello", &result);
  std::cout << "result is " << result << std::endl;
  Py_RETURN_NONE;
}

static PyMethodDef RayClientMethods[] = {
  { "connect", connect, METH_VARARGS, "connect to the shell" },
  { NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC initlibrayclient(void) {
  int argc = 1;
  const char* argv[] = { "rayclient", NULL };
  base::CommandLine::Init(argc, argv);
  shell::InitializeLogging();
  Py_InitModule3("librayclient", RayClientMethods, "ray-core python client library");
}

}
