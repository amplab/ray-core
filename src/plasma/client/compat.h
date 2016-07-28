#ifndef PYTHON_COMPAT_H
#define PYTHON_COMPAT_H

#include <Python.h>

#if PY_MAJOR_VERSION < 3
static inline PyObject *
PyMemoryView_FromMemory(char *mem, Py_ssize_t size, int flags) {
  PyMemoryViewObject *mview;
  int readonly;

  mview = (PyMemoryViewObject *)
      PyObject_GC_New(PyMemoryViewObject, &PyMemoryView_Type);
  if (mview == NULL)
    return NULL;
  mview->base = NULL;

  readonly = (flags == PyBUF_WRITE) ? 0 : 1;
  (void)PyBuffer_FillInfo(&mview->view, NULL, mem, size, readonly,
                          PyBUF_FULL_RO);

  _PyObject_GC_TRACK(mview);
  return (PyObject *)mview;
}
#endif

#endif // PYTHON_COMPAT_H
