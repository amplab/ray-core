#include <iostream>

#include <thread>
#include <string>
#include <Python.h>
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/md5.h"
#include "mojo/edk/util/make_unique.h"
#include "shell/init.h"
#include "examples/mybench/echo.mojom-sync.h"
#include "examples/mybench/echo.mojom.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"

#include "object_id.h"

std::thread start_rayclient(const char* c_child_connection_id, mojo::SynchronousInterfacePtr<mojo::examples::Echo>* result);

static std::thread global_thread;
static mojo::SynchronousInterfacePtr<mojo::examples::Echo> echo_interface;

extern "C" {

static PyObject* connect(PyObject* self, PyObject* args) {
  const char* address;
  const char* child_connection_id;
  if (!PyArg_ParseTuple(args, "ss", &address, &child_connection_id)) {
    return NULL;
  }
  global_thread = start_rayclient(child_connection_id, &echo_interface);
  Py_RETURN_NONE;
}

static PyObject* echo(PyObject* self, PyObject* args) {
  uint32 out = 0;
  echo_interface->EchoString(42, &out);
  return PyInt_FromLong(out);
}

static PyObject* build_object(PyObject* self, PyObject* args) {
  mojo::ScopedSharedBufferHandle handle;
  echo_interface->BuildObject(0, 200u, &handle);
  void* pointer = nullptr;
  MOJO_CHECK(MOJO_RESULT_OK == mojo::MapBuffer(handle.get(), 0, 100u, &pointer, MOJO_MAP_BUFFER_FLAG_NONE));
  return PyBuffer_FromReadWriteMemory(pointer, 100);
}

static PyObject* hash_task(PyObject* self, PyObject* args) {
  const char* name;
  int length;
  PyObject* arguments;
  if (!PyArg_ParseTuple(args, "s#O", &name, &length, &arguments)) {
  }
  base::MD5Context context;
  base::MD5Init(&context);
  base::MD5Update(&context, name);
  if (PyList_Check(arguments)) {
    for (size_t i = 0, size = PyList_Size(arguments); i < size; ++i) {
      PyObject* element = PyList_GetItem(arguments, i);
      if (PyObject_IsInstance(element, (PyObject*)&PyObjectIDType)) {
        ObjectID id = ((PyObjectID*) element)->id;
        base::MD5Update(&context, id.slice());
      } else {
        return NULL;
      }
    }
  }
  base::MD5Digest digest;
  base::MD5Final(&digest, &context);
  return PyObjectID_make(digest);
}

static PyObject* list_objects(PyObject* self, PyObject* args) {
  mojo::Array<mojo::examples::ObjectInfoPtr> result;
  echo_interface->ListObjects(&result);
  for (size_t i = 0; i < result.size(); ++i) {
    std::cout << result[i]->ms_since_epoch << std::endl;
  }
  Py_RETURN_NONE;
}

static PyMethodDef RayLibMethods[] = {
  { "connect", connect, METH_VARARGS, "connect to the shell" },
  { "build_object", build_object, METH_VARARGS, "get buffer to build an object" },
  { "hash_task", hash_task, METH_VARARGS, "" },
  { "list_objects", list_objects, METH_VARARGS, "list objects in the pool" },
  { "echo", echo, METH_VARARGS, "invoke call" },
  { NULL, NULL, 0, NULL }
};

struct ray_client_state {
  std::unique_ptr<base::AtExitManager> at_exit;
};

static ray_client_state global_state;

PyMODINIT_FUNC initlibrayclient(void) {
	// base::AtExitManager at_exit;
  global_state.at_exit = mojo::util::MakeUnique<base::AtExitManager>();
  int argc = 1;
  const char* argv[] = { "rayclient", NULL };
  base::CommandLine::Init(argc, argv);
  shell::InitializeLogging();
  PyObjectIDType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PyObjectIDType) < 0) {
    return;
  }
  PyObject* m = Py_InitModule3("librayclient", RayLibMethods, "ray-core python client library");
  Py_INCREF(&PyObjectIDType);
  PyModule_AddObject(m, "ObjectID", (PyObject *)&PyObjectIDType);
}

}
