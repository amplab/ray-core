#include "numpy.h"

#include <numbuf/tensor.h>

using namespace arrow;

namespace numbuf {

#define ARROW_TYPE_TO_NUMPY_CASE(TYPE) \
  case Type::TYPE:                     \
    return NPY_##TYPE;

#define DESERIALIZE_ARRAY_CASE(TYPE, ArrayType, type)                        \
  case Type::TYPE: {                                                         \
    auto values = std::dynamic_pointer_cast<ArrayType>(content->values());   \
    DCHECK(values);                                                          \
    type* data = const_cast<type*>(values->raw_data())                       \
                   + content->offset(offset);                                \
    *out = PyArray_SimpleNewFromData(num_dims, dim.data(), NPY_##TYPE,       \
                                       reinterpret_cast<void*>(data));       \
  }                                                                          \
  return Status::OK();

Status DeserializeArray(std::shared_ptr<Array> array, int32_t offset, PyObject** out) {
  DCHECK(array);
  auto tensor = std::dynamic_pointer_cast<StructArray>(array);
  DCHECK(tensor);
  auto dims = std::dynamic_pointer_cast<ListArray>(tensor->field(0));
  auto content = std::dynamic_pointer_cast<ListArray>(tensor->field(1));
  npy_intp num_dims = dims->value_length(offset);
  std::vector<npy_intp> dim(num_dims);
  for (int i = dims->offset(offset); i < dims->offset(offset+1); ++i) {
    dim[i - dims->offset(offset)] =
      std::dynamic_pointer_cast<Int64Array>(dims->values())->Value(i);
  }
  switch (content->value_type()->type) {
    DESERIALIZE_ARRAY_CASE(INT8, Int8Array, int8_t)
    DESERIALIZE_ARRAY_CASE(INT16, Int16Array, int16_t)
    DESERIALIZE_ARRAY_CASE(INT32, Int32Array, int32_t)
    DESERIALIZE_ARRAY_CASE(INT64, Int64Array, int64_t)
    DESERIALIZE_ARRAY_CASE(UINT8, UInt8Array, uint8_t)
    DESERIALIZE_ARRAY_CASE(UINT16, UInt16Array, uint16_t)
    DESERIALIZE_ARRAY_CASE(UINT32, UInt32Array, uint32_t)
    DESERIALIZE_ARRAY_CASE(UINT64, UInt64Array, uint64_t)
    DESERIALIZE_ARRAY_CASE(FLOAT, FloatArray, float)
    DESERIALIZE_ARRAY_CASE(DOUBLE, DoubleArray, double)
    default:
      DCHECK(false) << "arrow type not recognized: " << content->value_type()->type;
  }
  return Status::OK();
}

Status SerializeArray(PyArrayObject* array, SequenceBuilder& builder) {
  size_t ndim = PyArray_NDIM(array);
  int dtype = PyArray_TYPE(array);
  std::vector<int64_t> dims(ndim);
  for (int i = 0; i < ndim; ++i) {
    dims[i] = PyArray_DIM(array, i);
  }
  auto data = PyArray_DATA(array);
  switch (dtype) {
    case NPY_UINT8:
      return builder.Append(dims, reinterpret_cast<uint8_t*>(data));
    case NPY_INT8:
      return builder.Append(dims, reinterpret_cast<int8_t*>(data));
    case NPY_UINT16:
      return builder.Append(dims, reinterpret_cast<uint16_t*>(data));
    case NPY_INT16:
      return builder.Append(dims, reinterpret_cast<int16_t*>(data));
    case NPY_UINT32:
      return builder.Append(dims, reinterpret_cast<uint32_t*>(data));
    case NPY_INT32:
      return builder.Append(dims, reinterpret_cast<int32_t*>(data));
    case NPY_UINT64:
      return builder.Append(dims, reinterpret_cast<uint64_t*>(data));
    case NPY_INT64:
      return builder.Append(dims, reinterpret_cast<int64_t*>(data));
    case NPY_FLOAT:
      return builder.Append(dims, reinterpret_cast<float*>(data));
    case NPY_DOUBLE:
      return builder.Append(dims, reinterpret_cast<double*>(data));
    default:
      DCHECK(false) << "numpy data type not recognized: " << dtype;
  }
  return Status::OK();
}

}
