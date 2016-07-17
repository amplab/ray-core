#ifndef NUMBUF_TENSOR_H
#define NUMBUF_TENSOR_H

#include <memory>
#include <arrow/type.h>
#include <arrow/api.h>

namespace numbuf {

/*! This is a class for building a dataframe where each row corresponds to
    a Tensor (= multidimensional array) of numerical data. There are two
    columns, "dims" which contains an array of dimensions for each Tensor
    and "data" which contains data buffer of the Tensor as a flattened array.
*/
template<typename T>
class TensorBuilder {
public:
  typedef typename T::c_type elem_type;

  TensorBuilder(const arrow::TypePtr& dtype, arrow::MemoryPool* pool = nullptr);

  /*! Append a new tensor.

      \param dims
        The dimensions of the Tensor

      \param data
        Pointer to the beginning of the data buffer of the Tensor. The
        total length of the buffer is sizeof(elem_type) * product of dims[i] over i
  */
  arrow::Status Append(const std::vector<int64_t>& dims, const elem_type* data);

  //! Convert the tensors to an Arrow StructArray
  std::shared_ptr<arrow::Array> Finish();

  //! Number of tensors in the column
  int32_t length() {
    return tensors_->length();
  }

  const arrow::TypePtr& type() {
    return tensors_->type();
  }

private:
	arrow::TypePtr dtype_;
  std::shared_ptr<arrow::Int64Builder> dim_data_;
  std::shared_ptr<arrow::ListBuilder> dims_;
  std::shared_ptr<arrow::PrimitiveBuilder<T>> value_data_;
  std::shared_ptr<arrow::ListBuilder> values_;
  std::shared_ptr<arrow::StructBuilder> tensors_;
};


typedef TensorBuilder<arrow::DoubleType> DoubleTensorBuilder;

}

#endif // NUMBUF_TENSOR_H
