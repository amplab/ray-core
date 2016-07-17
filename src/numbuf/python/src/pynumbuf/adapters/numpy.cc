#include "numpy.h"

#include <numbuf/tensor.h>
// #include <numbuf/types.h>

using namespace arrow;

namespace numbuf {

const auto BOOL_TYPE = std::make_shared<arrow::BooleanType>();

const auto INT8_TYPE = std::make_shared<arrow::Int8Type>();
const auto INT16_TYPE = std::make_shared<arrow::Int16Type>();
const auto INT32_TYPE = std::make_shared<arrow::Int32Type>();
const auto INT64_TYPE = std::make_shared<arrow::Int64Type>();

const auto UINT8_TYPE = std::make_shared<arrow::UInt8Type>();
const auto UINT16_TYPE = std::make_shared<arrow::UInt16Type>();
const auto UINT32_TYPE = std::make_shared<arrow::UInt32Type>();
const auto UINT64_TYPE = std::make_shared<arrow::UInt64Type>();

const auto FLOAT_TYPE = std::make_shared<arrow::FloatType>();
const auto DOUBLE_TYPE = std::make_shared<arrow::DoubleType>();

#define NUMPY_TYPE_TO_ARROW_CASE(TYPE)     \
  case NPY_##TYPE:                         \
    return TYPE##_TYPE;

TypePtr numpy_type_to_arrow(int numpy_type) {
  switch (numpy_type) {
    NUMPY_TYPE_TO_ARROW_CASE(INT8)
    NUMPY_TYPE_TO_ARROW_CASE(INT16)
    NUMPY_TYPE_TO_ARROW_CASE(INT32)
    NUMPY_TYPE_TO_ARROW_CASE(INT64)
    NUMPY_TYPE_TO_ARROW_CASE(UINT8)
    NUMPY_TYPE_TO_ARROW_CASE(UINT16)
    NUMPY_TYPE_TO_ARROW_CASE(UINT32)
    NUMPY_TYPE_TO_ARROW_CASE(UINT64)
    NUMPY_TYPE_TO_ARROW_CASE(FLOAT)
    NUMPY_TYPE_TO_ARROW_CASE(DOUBLE)
    default:
      assert(false);
  }
}

#define ARROW_TYPE_TO_NUMPY_CASE(TYPE) \
  case Type::TYPE:                     \
    return NPY_##TYPE;

int arrow_type_to_numpy(TypePtr type) {
  switch (type->type) {
    ARROW_TYPE_TO_NUMPY_CASE(INT8)
    ARROW_TYPE_TO_NUMPY_CASE(INT16)
    ARROW_TYPE_TO_NUMPY_CASE(INT32)
    ARROW_TYPE_TO_NUMPY_CASE(INT64)
    ARROW_TYPE_TO_NUMPY_CASE(UINT8)
    ARROW_TYPE_TO_NUMPY_CASE(UINT16)
    ARROW_TYPE_TO_NUMPY_CASE(UINT32)
    ARROW_TYPE_TO_NUMPY_CASE(UINT64)
    ARROW_TYPE_TO_NUMPY_CASE(FLOAT)
    ARROW_TYPE_TO_NUMPY_CASE(DOUBLE)
    default:
      assert(false);
  }
}

Status DeserializeArray(std::shared_ptr<Array> array, int32_t offset, PyObject** out) {
  DCHECK(array);
  auto tensor = std::dynamic_pointer_cast<StructArray>(array);
  DCHECK(tensor);
  auto dims = std::dynamic_pointer_cast<ListArray>(tensor->field(0));
  auto content = std::dynamic_pointer_cast<ListArray>(tensor->field(1));
  auto values = std::dynamic_pointer_cast<DoubleArray>(content->values());
  double* data = const_cast<double*>(values->raw_data()) + content->offset(offset);
  npy_intp num_dims = dims->value_length(offset);
  std::vector<npy_intp> dim(num_dims);
  int j = 0; // TODO(pcm): make this the loop variable
  for (int i = dims->offset(offset); i < dims->offset(offset+1); ++i) {
    dim[j] = std::dynamic_pointer_cast<Int64Array>(dims->values())->Value(i);
    j += 1;
  }
  *out = PyArray_SimpleNewFromData(num_dims, dim.data(), arrow_type_to_numpy(values->type()), reinterpret_cast<void*>(data));
  return Status::OK();
}

Status SerializeArray(PyArrayObject* array, SequenceBuilder& builder) {
  size_t ndim = PyArray_NDIM(array);
  int dtype = PyArray_TYPE(array);
  std::vector<int64_t> dims(ndim);
  for (int i = 0; i < ndim; ++i) {
    dims[i] = PyArray_DIM(array, i);
  }
  auto type = numpy_type_to_arrow(dtype);
  auto data = reinterpret_cast<double*>(PyArray_DATA(array));
  return builder.Append(dims, data);
}

}
