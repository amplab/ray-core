#include "python.h"

using namespace arrow;

namespace numbuf {

PyObject* get_value(ArrayPtr arr, int32_t index, int32_t type) {
  PyObject* result;
  switch (type) {
    case BOOL_TAG:
      return PyBool_FromLong(std::static_pointer_cast<BooleanArray>(arr)->Value(index));
    case INT_TAG:
      return PyInt_FromLong(std::static_pointer_cast<Int64Array>(arr)->Value(index));
    case STRING_TAG: {
      int32_t nchars;
      const uint8_t* str = std::static_pointer_cast<StringArray>(arr)->GetValue(index, &nchars);
      return PyString_FromStringAndSize(reinterpret_cast<const char*>(str), nchars);
    }
    case FLOAT_TAG:
      return PyFloat_FromDouble(std::static_pointer_cast<FloatArray>(arr)->Value(index));
    case DOUBLE_TAG:
      return PyFloat_FromDouble(std::static_pointer_cast<DoubleArray>(arr)->Value(index));
    case LIST_TAG: {
      auto list = std::static_pointer_cast<ListArray>(arr);
      ARROW_CHECK_OK(DeserializeList(list->values(), list->value_offset(index), list->value_offset(index+1), &result));
      return result;
    }
    case DICT_TAG: {
      auto list = std::static_pointer_cast<ListArray>(arr);
      ARROW_CHECK_OK(DeserializeDict(list->values(), list->value_offset(index), list->value_offset(index+1), &result));
      return result;
    }
    case TENSOR_TAG:
      ARROW_CHECK_OK(DeserializeArray(arr, index, &result));
      return result;
    default:
      DCHECK(false) << "union tag not recognized " << type;
  }
  return NULL;
}

Status append(PyObject* elem, SequenceBuilder& builder, std::vector<PyObject*>& sublists, std::vector<PyObject*>& subdicts) {
  // The bool case must precede the int case (PyInt_Check passes for bools)
  if (PyBool_Check(elem)) {
    RETURN_NOT_OK(builder.Append(elem == Py_True));
  } else if (PyFloat_Check(elem)) {
    RETURN_NOT_OK(builder.Append(PyFloat_AS_DOUBLE(elem)));
  } else if (PyInt_Check(elem)) {
    RETURN_NOT_OK(builder.Append(PyInt_AS_LONG(elem)));
  } else if (PyString_Check(elem)) {
    RETURN_NOT_OK(builder.Append(PyString_AS_STRING(elem)));
  } else if (PyList_Check(elem)) {
    builder.AppendList(PyList_Size(elem));
    sublists.push_back(elem);
  } else if (PyDict_Check(elem)) {
    builder.AppendDict(PyDict_Size(elem));
    subdicts.push_back(elem);
  } else if (PyArray_Check(elem)) {
    RETURN_NOT_OK(SerializeArray((PyArrayObject*) elem, builder));
  } else if (elem == Py_None) {
    RETURN_NOT_OK(builder.Append());
  } else {
    DCHECK(false) << "data type of " << PyString_AS_STRING(PyObject_Repr(elem))
                  << " not recognized";
  }
  return Status::OK();
}

std::shared_ptr<Array> SerializeDict(std::vector<PyObject*> dicts) {
  DictBuilder result;
  std::vector<PyObject*> sublists, subdicts, dummy;
  for (const auto& dict : dicts) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
      ARROW_CHECK_OK(append(key, result.keys(), dummy, dummy));
      ARROW_CHECK_OK(append(value, result.vals(), sublists, subdicts));
    }
  }
  auto val_list = sublists.size() > 0 ? SerializeList(sublists) : nullptr;
  auto val_dict = subdicts.size() > 0 ? SerializeDict(subdicts) : nullptr;
  return result.Finish(val_list, val_dict);
}

std::shared_ptr<Array> SerializeList(std::vector<PyObject*> lists) {
  SequenceBuilder builder(nullptr);
  std::vector<PyObject*> sublists, subdicts;
  for (const auto& list : lists) {
    for (size_t i = 0, size = PyList_Size(list); i < size; ++i) {
      PyObject* elem = PyList_GetItem(list, i);
      ARROW_CHECK_OK(append(elem, builder, sublists, subdicts));
    }
  }
  auto list = sublists.size() > 0 ? SerializeList(sublists) : nullptr;
  auto dict = subdicts.size() > 0 ? SerializeDict(subdicts) : nullptr;
  return builder.Finish(list, dict);
}

Status DeserializeList(std::shared_ptr<Array> array, int32_t start_idx, int32_t stop_idx, PyObject** out) {
  auto data = std::dynamic_pointer_cast<DenseUnionArray>(array);
  // TODO(pcm): error handling
  int32_t size = array->length();
  PyObject* result = PyList_New(stop_idx - start_idx);
  auto types = std::make_shared<Int8Array>(size, data->types());
  auto offsets = std::make_shared<Int32Array>(size, data->offset_buf());
  for (size_t i = start_idx; i < stop_idx; ++i) {
    if (data->IsNull(i)) {
      Py_INCREF(Py_None);
      PyList_SetItem(result, i-start_idx, Py_None);
    } else {
      int32_t offset = offsets->Value(i);
      int8_t type = types->Value(i);
      ArrayPtr arr = data->child(type);
      PyList_SetItem(result, i-start_idx, get_value(arr, offset, type));
    }
  }
  *out = result;
  return Status::OK();
}

Status DeserializeDict(std::shared_ptr<Array> array, int32_t start_idx, int32_t stop_idx, PyObject** out) {
  auto data = std::dynamic_pointer_cast<StructArray>(array);
  // TODO(pcm): error handling, get rid of the temporary copy of the list
  PyObject *keys, *vals;
  PyObject* result = PyDict_New();
  ARROW_RETURN_NOT_OK(DeserializeList(data->field(0), start_idx, stop_idx, &keys));
  ARROW_RETURN_NOT_OK(DeserializeList(data->field(1), start_idx, stop_idx, &vals));
  for (size_t i = start_idx; i < stop_idx; ++i) {
    PyDict_SetItem(result, PyList_GetItem(keys, i - start_idx), PyList_GetItem(vals, i - start_idx));
  }
  Py_XDECREF(keys); // PyList_GetItem(keys, ...) incremented the reference count
  Py_XDECREF(vals); // PyList_GetItem(vals, ...) incremented the reference count
  *out = result;
  return Status::OK();
}


}
