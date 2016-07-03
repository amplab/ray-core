#include <Python.h>
#include "base/md5.h"

int _PyLong_AsByteArray(PyLongObject* v,
                        unsigned char* bytes, size_t n,
                        int little_endian, int is_signed);

class ObjectID {
public:
  ObjectID(base::MD5Digest id) : id_(id) {}
  uint8_t* data() {
    return &id_.a[0];
  }
  size_t size() {
    return sizeof(base::MD5Digest);
  }
  std::string repr() {
    return base::MD5DigestToBase16(id_);
  }
  base::StringPiece slice() {
    return base::StringPiece((char*) data(), size());
  }
private:
  base::MD5Digest id_;
};

// #define OBJECT_ID_ADDR(OBJECT_ID) OBJECT_ID.a

typedef struct {
    PyObject_HEAD
    ObjectID id;
} PyObjectID;

static void PyObjectID_dealloc(PyObjectID *self) {
  self->ob_type->tp_free((PyObject*) self);
}

static PyObject* PyObjectID_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PyObjectID* self = (PyObjectID*) type->tp_alloc(type, 0);
  if (self != NULL) {
    memset(self->id.data(), 0, self->id.size());
  }
  return (PyObject*) self;
}

static int PyObjectID_init(PyObjectID *self, PyObject *args, PyObject *kwds) {
  PyObject* data;
  if (!PyArg_ParseTuple(args, "O", &data)) {
    return -1;
  }
  if (PyInt_Check(data)) {
    data = PyLong_FromLong(PyInt_AsLong(data)); // TODO(pcm): memory leak
  }
  if (!PyLong_Check(data)) {
    return -1;
  }
  return _PyLong_AsByteArray((PyLongObject*) data,
      self->id.data(), self->id.size(), 0, 0);
};

static int PyObjectID_compare(PyObject* a, PyObject* b) {
  PyObjectID* A = (PyObjectID*) a;
  PyObjectID* B = (PyObjectID*) b;
  return memcmp(&A->id, &B->id, sizeof(ObjectID));
}

static PyObject* PyObjectID_repr(PyObject* self) {
  PyObjectID* id = (PyObjectID*) self;
  return PyString_FromString(id->id.repr().c_str());
}

static PyTypeObject PyObjectIDType = {
  PyObject_HEAD_INIT(NULL)
  0,                         /* ob_size */
  "ObjectID",                /* tp_name */
  sizeof(PyObjectID),        /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)PyObjectID_dealloc,          /* tp_dealloc */
  0,                         /* tp_print */
  0,                         /* tp_getattr */
  0,                         /* tp_setattr */
  PyObjectID_compare,          /* tp_compare */
  PyObjectID_repr,           /* tp_repr */
  0,                         /* tp_as_number */
  0,                         /* tp_as_sequence */
  0,                         /* tp_as_mapping */
  0,                         /* tp_hash */
  0,                         /* tp_call */
  0,                         /* tp_str */
  0,                         /* tp_getattro */
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,        /* tp_flags */
  "Ray objects",            /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  0,                         /* tp_methods */
  0,          /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)PyObjectID_init,   /* tp_init */
  0,                         /* tp_alloc */
  PyObjectID_new,              /* tp_new */
};

PyObject* PyObjectID_make(base::MD5Digest id) {
  PyObjectID* result = (PyObjectID*) PyObjectID_new(&PyObjectIDType, NULL, NULL);
  result->id = ObjectID(id);
  return (PyObject*) result;
}
