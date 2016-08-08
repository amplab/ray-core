#ifndef PYNUMBUF_PYTHON_H
#define PYNUMBUF_PYTHON_H

#include <Python.h>

#include <arrow/api.h>
#include <numbuf/dict.h>
#include <numbuf/sequence.h>

#include "numpy.h"

namespace numbuf {

std::shared_ptr<arrow::Array> SerializeList(std::vector<PyObject*> list);
arrow::Status DeserializeList(std::shared_ptr<arrow::Array> array, int32_t start_idx, int32_t stop_idx, PyObject** out);
arrow::Status DeserializeDict(std::shared_ptr<arrow::Array> array, int32_t start_idx, int32_t stop_idx, PyObject** out);

}

#endif
