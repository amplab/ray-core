#ifndef NUMBUF_LIST_H
#define NUMBUF_LIST_H

#include <arrow/api.h>
#include <arrow/types/union.h>
#include "tensor.h"

namespace numbuf {

const int8_t BOOL_TAG = 0;
const int8_t INT_TAG = 1;
const int8_t STRING_TAG = 2;
const int8_t FLOAT_TAG = 3;
const int8_t DOUBLE_TAG = 4;
const int8_t TENSOR_TAG = 5;
const int8_t LIST_TAG = 6;
const int8_t TUPLE_TAG = 7;
const int8_t DICT_TAG = 8;

const int8_t NUM_TAGS = 9;

class SequenceBuilder {
 public:
  SequenceBuilder(arrow::MemoryPool* pool = nullptr);

  //! Appending a none to the list
  arrow::Status Append();

  //! Appending a boolean to the list
  arrow::Status Append(bool data);

  //! Appending an int64_t to the list
  arrow::Status Append(int64_t data);

  //! Appending a null-delimited string to the list
  arrow::Status Append(const char* data);

  //! Appending a C++ string to the list
  arrow::Status Append(const std::string& data);

  //! Appending a float to the list
  arrow::Status Append(float data);

  //! Appending a double to the list
  arrow::Status Append(double data);

  /*! Appending a tensor to the list

      \param dims
        A vector of dimensions

      \param data
        A pointer to the start of the data block. The length of the data block
        will be the product of the dimensions
  */
  arrow::Status Append(const std::vector<int64_t>& dims, double* data);

  /*! Add a sublist to the list. The data contained in the list will be
     specified in the "Finish" method.

     To construct l = [[11, 22], 33, [44, 55]] you would for example run
     list = ListBuilder();
     list.AppendList(2);
     list.Append(33);
     list.AppendList(2);
     list.Finish([11, 22, 44, 55]);
     list.Finish();

     \param size
       The size of the sublist
  */
  arrow::Status AppendList(int32_t size);

  arrow::Status AppendTuple(int32_t size);

  arrow::Status AppendDict(int32_t size);

  //! Finish building the list and return the result
  std::shared_ptr<arrow::DenseUnionArray> Finish(
    std::shared_ptr<arrow::Array> list_data,
    std::shared_ptr<arrow::Array> tuple_data,
    std::shared_ptr<arrow::Array> dict_data);

 private:
  arrow::MemoryPool* pool_;

  arrow::Int8Builder types_;
  arrow::Int32Builder offsets_;

  arrow::NullArrayBuilder nones_;
  arrow::BooleanBuilder bools_;
  arrow::Int64Builder ints_;
  arrow::StringBuilder strings_;
  arrow::FloatBuilder floats_;
  arrow::DoubleBuilder doubles_;
  DoubleTensorBuilder tensors_;
  std::vector<int32_t> list_offsets_;
  std::vector<int32_t> tuple_offsets_;
  std::vector<int32_t> dict_offsets_;
};

} // namespace numbuf

#endif // NUMBUF_LIST_H
