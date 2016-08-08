#include <string>
#include <Python.h>
#include "base/command_line.h"
#include "shell/init.h"
#include "plasma/service/api.h"

extern "C" {

using plasma::ClientContext;
using plasma::MutableBuffer;
using plasma::ObjectID;

static int GetClientContext(PyObject* object, ClientContext** context) {
  if (PyCapsule_IsValid(object, "plasma")) {
    *context = static_cast<ClientContext*>(PyCapsule_GetPointer(object, "plasma"));
    return 1;
  } else {
    PyErr_SetString(PyExc_TypeError, "must be a 'plasma' capsule");
    return 0;
  }
}

static void PlasmaCapsule_Destructor(PyObject* capsule) {
  delete static_cast<ClientContext*>(PyCapsule_GetPointer(capsule, "plasma"));
}

static int GetMutableBuffer(PyObject* object, MutableBuffer** buffer) {
  if (PyCapsule_IsValid(object, "mut_buff")) {
    *buffer = static_cast<MutableBuffer*>(PyCapsule_GetPointer(object, "mut_buff"));
    return 1;
  } else {
    PyErr_SetString(PyExc_TypeError, "must be a 'mut_buff' capsule");
    return 0;
  }
}

static void MutableBufferCapsule_Destructor(PyObject* capsule) {
  delete static_cast<MutableBuffer*>(PyCapsule_GetPointer(capsule, "mut_buff"));
}

static PyObject* connect(PyObject* self, PyObject* args) {
  const char* address;
  if (!PyArg_ParseTuple(args, "s", &address)) {
    return NULL;
  }
  auto context = new ClientContext(std::string(address));
  return PyCapsule_New(context, "plasma", PlasmaCapsule_Destructor);
}

static PyObject* build_object(PyObject* self, PyObject* args) {
  ClientContext* context;
  ObjectID object_id;
  Py_ssize_t size;
  const char* name;
  if (!PyArg_ParseTuple(args, "O&nns", &GetClientContext,
                        &context, &object_id, &size, &name)) {
    return NULL;
  }
  auto mutable_buffer = new MutableBuffer();
  context->BuildObject(object_id, size, *mutable_buffer, std::string(name));
  return PyCapsule_New(mutable_buffer, "mut_buff", MutableBufferCapsule_Destructor);
}

static PyObject* get_mutable_buffer(PyObject* self, PyObject* args) {
  MutableBuffer* buffer;
  if (!PyArg_ParseTuple(args, "O&", &GetMutableBuffer, &buffer)) {
    return NULL;
  }
  return PyBuffer_FromReadWriteMemory(reinterpret_cast<void*>(buffer->mutable_data()), buffer->size());
}

static PyObject* seal_object(PyObject* self, PyObject* args) {
  MutableBuffer* buffer;
  if (!PyArg_ParseTuple(args, "O&", &GetMutableBuffer, &buffer)) {
    return NULL;
  }
  buffer->Seal();
  Py_RETURN_NONE;
}

// TODO: implement blocking and nonblocking version of this
static PyObject* get_object(PyObject* self, PyObject* args) {
  ClientContext* context;
  ObjectID object_id;
  if (!PyArg_ParseTuple(args, "O&n", &GetClientContext, &context, &object_id)) {
    return NULL;
  }
  plasma::Buffer buffer;
  context->GetObject(object_id, buffer);
  const void* data = reinterpret_cast<const void*>(buffer.data());
  // We need the const cast because the Python API does not implement const for this method
  // TODO(pcm): Maybe the new Python buffer API does?
  return PyBuffer_FromMemory(const_cast<void*>(data), buffer.size());
}

static PyObject* list_objects(PyObject* self, PyObject* args) {
  ClientContext* context;
  if (!PyArg_ParseTuple(args, "O&", &GetClientContext, &context)) {
    return NULL;
  }
  std::vector<plasma::ObjectInfo> infos;
  context->ListObjects(&infos);
  PyObject* names = PyList_New(infos.size());
  PyObject* sizes = PyList_New(infos.size());
  PyObject* create_times = PyList_New(infos.size());
  PyObject* construct_deltas = PyList_New(infos.size());
  PyObject* creator_ids = PyList_New(infos.size());
  for (size_t i = 0; i < infos.size(); ++i) {
    PyList_SetItem(names, i, PyString_FromString(infos[i].name.c_str()));
    PyList_SetItem(sizes, i, PyInt_FromLong(infos[i].size));
    PyList_SetItem(create_times, i, PyInt_FromLong(infos[i].create_time));
    PyList_SetItem(construct_deltas, i, PyInt_FromLong(infos[i].construct_delta));
    PyList_SetItem(creator_ids, i, PyInt_FromLong(infos[i].creator_id));
  }
  PyObject* result = PyTuple_New(5);
  PyTuple_SetItem(result, 0, names);
  PyTuple_SetItem(result, 1, sizes);
  PyTuple_SetItem(result, 2, create_times);
  PyTuple_SetItem(result, 3, construct_deltas);
  PyTuple_SetItem(result, 4, creator_ids);
  return result;
}

static PyMethodDef RayClientMethods[] = {
  { "connect", connect, METH_VARARGS, "connect to the shell" },
  { "build_object", build_object, METH_VARARGS, "build a new object" },
  { "get_mutable_buffer", get_mutable_buffer, METH_VARARGS, "get mutable buffer" },
  { "seal_object", seal_object, METH_VARARGS, "seal an object" },
  { "get_object", get_object, METH_VARARGS, "get an object from plasma" },
  { "list_objects", list_objects, METH_VARARGS, "list objects in plasma" },
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
