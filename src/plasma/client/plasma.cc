#include <iostream>

#include <string>
#include <thread>
#include <Python.h>
#include "base/at_exit.h"
#include "base/command_line.h"
#include "mojo/edk/util/make_unique.h"
#include "shell/init.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"

#include "plasma/service/plasma.mojom-sync.h"
#include "ray/client/client_context.h"

extern "C" {

typedef int64_t ObjectID;

typedef mojo::SynchronousInterfacePtr<plasma::Plasma> PlasmaInterface;

struct PlasmaContext {
  shell::ClientContext<plasma::Plasma> context;
  PlasmaInterface interface;
};

static int GetPlasmaInterface(PyObject* object, PlasmaInterface** interface) {
  if (PyCapsule_IsValid(object, "plasma")) {
    auto p = static_cast<PlasmaContext*>(PyCapsule_GetPointer(object, "plasma"));
    *interface = &p->interface;
    return 1;
  } else {
    PyErr_SetString(PyExc_TypeError, "must be a 'plasma' capsule");
    return 0;
  }
}

static void PlasmaCapsule_Destructor(PyObject* capsule) {
  delete static_cast<PlasmaContext*>(PyCapsule_GetPointer(capsule, "plasma"));
}

static PyObject* connect(PyObject* self, PyObject* args) {
  const char* address;
  const char* child_connection_id;
  if (!PyArg_ParseTuple(args, "ss", &address, &child_connection_id)) {
    return NULL;
  }
  auto context = std::unique_ptr<PlasmaContext>(new PlasmaContext());
  context->context.ConnectToShell(std::string("mojo:plasma"), std::string(address),
                                  std::string(child_connection_id));
  context->interface = context->context.GetInterface();
  return PyCapsule_New(context.release(), "plasma", PlasmaCapsule_Destructor);
}

static PyObject* build_object(PyObject* self, PyObject* args) {
  PlasmaInterface* plasma;
  ObjectID object_id;
  Py_ssize_t size;
  const char* name;
  if (!PyArg_ParseTuple(args, "O&nns", &GetPlasmaInterface,
                        &plasma, &object_id, &size, &name)) {
    return NULL;
  }
  mojo::ScopedSharedBufferHandle handle;
  (*plasma)->BuildObject(object_id, size, std::string(name), &handle);
  void* pointer = nullptr;
  CHECK_EQ(MOJO_RESULT_OK, mojo::MapBuffer(handle.get(), 0, size, &pointer, MOJO_MAP_BUFFER_FLAG_NONE));
  return PyBuffer_FromReadWriteMemory(pointer, size);
}

static PyObject* seal_object(PyObject* self, PyObject* args) {
  PlasmaInterface* plasma;
  ObjectID object_id;
  if (!PyArg_ParseTuple(args, "O&n", &GetPlasmaInterface, &plasma, &object_id)) {
    return NULL;
  }
  (*plasma)->SealObject(object_id);
  Py_RETURN_NONE;
}

// TODO: implement blocking and nonblocking version of this
static PyObject* get_object(PyObject* self, PyObject* args) {
  PlasmaInterface* plasma;
  ObjectID object_id;
  if (!PyArg_ParseTuple(args, "O&n", &GetPlasmaInterface, &plasma, &object_id)) {
    return NULL;
  }
  mojo::ScopedSharedBufferHandle handle;
  uint64_t size;
  (*plasma)->GetObject(object_id, true, &handle, &size);
  void* pointer = nullptr;
  CHECK_EQ(MOJO_RESULT_OK, mojo::MapBuffer(handle.get(), 0, size, &pointer, MOJO_MAP_BUFFER_FLAG_NONE));
  return PyBuffer_FromMemory(pointer, size);
}

static PyObject* list_object(PyObject* self, PyObject* args) {
  PlasmaInterface* plasma;
  if (!PyArg_ParseTuple(args, "O&", &GetPlasmaInterface, &plasma)) {
    return NULL;
  }
  mojo::Array<plasma::ObjectInfoPtr> infos;
  (*plasma)->ListObjects(&infos);
  PyObject* names = PyList_New(infos.size());
  PyObject* sizes = PyList_New(infos.size());
  PyObject* timestamps = PyList_New(infos.size());
  for (size_t i = 0; i < infos.size(); ++i) {
    PyList_SetItem(names, i, PyString_FromString(infos[i]->name.data()));
    PyList_SetItem(sizes, i, PyInt_FromLong(infos[i]->size));
    PyList_SetItem(timestamps, i, PyInt_FromLong(infos[i]->timestamp));
  }
  PyObject* result = PyTuple_New(3);
  PyTuple_SetItem(result, 0, names);
  PyTuple_SetItem(result, 1, sizes);
  PyTuple_SetItem(result, 2, timestamps);
  return result;
}

static PyMethodDef RayClientMethods[] = {
  { "connect", connect, METH_VARARGS, "connect to the shell" },
  { "build_object", build_object, METH_VARARGS, "build a new object" },
  { "seal_object", seal_object, METH_VARARGS, "seal an object" },
  { "get_object", get_object, METH_VARARGS, "get an object from plasma" },
  { "list_object", list_object, METH_VARARGS, "list objects in plasma" },
  { NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC initlibplasma(void) {
  int argc = 1;
  const char* argv[] = { "libplasma", NULL };
  base::CommandLine::Init(argc, argv);
  shell::InitializeLogging();
  Py_InitModule3("libplasma", RayClientMethods, "plasma python client library");
}

}
