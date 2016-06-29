#include <string>
#include <Python.h>
#include "base/at_exit.h"
#include "base/command_line.h"
#include "mojo/edk/util/make_unique.h"
#include "shell/init.h"

void start_rayclient(const std::string& address, const std::string& child_connection_id);

extern "C" {

static PyObject* connect(PyObject* self, PyObject* args) {
  const char* address;
  const char* child_connection_id;
  if (!PyArg_ParseTuple(args, "ss", &address, &child_connection_id)) {
    return NULL;
  }
  start_rayclient(std::string(address), std::string(child_connection_id));
  Py_RETURN_NONE;
}

static PyMethodDef RayLibMethods[] = {
  { "connect", connect, METH_VARARGS, "connect to the shell" },
  { NULL, NULL, 0, NULL }
};

struct ray_client_state {
  std::unique_ptr<base::AtExitManager> at_exit;
};

static ray_client_state global_state;

PyMODINIT_FUNC initlibrayclient(void) {
  global_state.at_exit = mojo::util::MakeUnique<base::AtExitManager>();
  int argc = 1;
  const char* argv[] = { "rayclient", NULL };
  base::CommandLine::Init(argc, argv);
  shell::InitializeLogging();
  Py_InitModule3("librayclient", RayLibMethods, "ray-core python client library");
}

}
