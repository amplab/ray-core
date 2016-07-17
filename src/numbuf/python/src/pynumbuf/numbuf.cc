#include <Python.h>
#include <arrow/api.h>
#include <arrow/ipc/memory.h>
#include <arrow/ipc/adapter.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL NUMBUF_ARRAY_API
#include <numpy/arrayobject.h>

#include <iostream>

#include "adapters/python.h"

using namespace arrow;
using namespace numbuf;

extern "C" {

int PyObjectToArrow(PyObject* object, std::shared_ptr<Array> **result) {
  if (PyCapsule_IsValid(object, "arrow")) {
    *result = static_cast<std::shared_ptr<Array>*>(PyCapsule_GetPointer(object, "arrow"));
    return 1;
  } else {
    PyErr_SetString(PyExc_TypeError, "must be an 'arrow' capsule");
    return 0;
  }
}

static void ArrowCapsule_Destructor(PyObject* capsule) {
  delete static_cast<std::shared_ptr<Array>*>(PyCapsule_GetPointer(capsule, "arrow"));
}

PyObject* serialize_list(PyObject* self, PyObject* args) {
  PyObject* value;
  if (!PyArg_ParseTuple(args, "O", &value)) {
    return NULL;
  }
  std::shared_ptr<Array>* result = new std::shared_ptr<Array>();
  if (PyList_Check(value)) {
    *result = SerializeSequences(std::vector<PyObject*>({value}));
    std::cout << "validation: " << (*result)->Validate().ToString() << std::endl;
    std::cout << "schema: " << (*result)->type()->ToString() << std::endl;
    return PyCapsule_New(static_cast<void*>(result), "arrow", &ArrowCapsule_Destructor);
  }
  return NULL;
}

PyObject* deserialize_list(PyObject* self, PyObject* args) {
  std::shared_ptr<Array>* data;
  if (!PyArg_ParseTuple(args, "O&", &PyObjectToArrow, &data)) {
    return NULL;
  }
  PyObject* result;
  ARROW_CHECK_OK(DeserializeList(*data, 0, (*data)->length(), &result));
  return result;
}

static PyMethodDef NumbufMethods[] = {
 { "serialize_list", serialize_list, METH_VARARGS, "serialize a Python list" },
 { "deserialize_list", deserialize_list, METH_VARARGS, "deserialize a Python list" },
 { NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC initlibnumbuf(void) {
  PyObject* m;
  m = Py_InitModule3("libnumbuf", NumbufMethods, "Python C Extension for Numbuf");
  import_array();
}

}
