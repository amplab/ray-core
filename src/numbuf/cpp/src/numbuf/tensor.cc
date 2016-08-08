#include "tensor.h"

using namespace arrow;

namespace numbuf {

template<typename T>
TensorBuilder<T>::TensorBuilder(const TypePtr& dtype, MemoryPool* pool)
    : dtype_(dtype) {
  dim_data_ = std::make_shared<Int64Builder>(pool);
  dims_ = std::make_shared<ListBuilder>(pool, dim_data_);
  value_data_ = std::make_shared<PrimitiveBuilder<T>>(pool, dtype);
  values_ = std::make_shared<ListBuilder>(pool, value_data_);
  auto dims_field = std::make_shared<Field>("dims", dims_->type());
  auto values_field = std::make_shared<Field>("data", values_->type());
  auto type = std::make_shared<StructType>(std::vector<FieldPtr>({dims_field, values_field}));
  tensors_ = std::make_shared<StructBuilder>(pool, type, std::vector<std::shared_ptr<ArrayBuilder>>({dims_, values_}));
};

template<typename T>
Status TensorBuilder<T>::Append(const std::vector<int64_t>& dims, const elem_type* data) {
  RETURN_NOT_OK(tensors_->Append());
  RETURN_NOT_OK(dims_->Append());
  RETURN_NOT_OK(values_->Append());
  int32_t size = 1;
  for (auto dim : dims) {
    size *= dim;
    RETURN_NOT_OK(dim_data_->Append(dim));
  }
  std::cout << "appended with argument " << data << std::endl;
  RETURN_NOT_OK(value_data_->Append(data, size));
  return Status::OK(); // tensors_->Append();
}

template<typename T>
std::shared_ptr<Array> TensorBuilder<T>::Finish() {
  return tensors_->Finish();
}

template class TensorBuilder<DoubleType>;

}
