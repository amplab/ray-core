#include "sequence.h"

using namespace arrow;

namespace numbuf {

SequenceBuilder::SequenceBuilder(MemoryPool* pool)
    : pool_(pool), types_(pool), offsets_(pool),
      nones_(pool, std::make_shared<NullType>()),
      bools_(pool, std::make_shared<BooleanType>()),
      ints_(pool), strings_(pool, std::make_shared<StringType>()),
      floats_(pool), doubles_(pool),
      tensors_(std::make_shared<DoubleType>(), pool),
      list_offsets_({0}), dict_offsets_({0}) {}

#define NUMBUF_LIST_UPDATE(OFFSET, TAG)          \
  	RETURN_NOT_OK(offsets_.Append(OFFSET));      \
    RETURN_NOT_OK(types_.Append(TAG));           \
    RETURN_NOT_OK(nones_.AppendToBitmap(true));

Status SequenceBuilder::Append() {
  RETURN_NOT_OK(offsets_.Append(0));
  RETURN_NOT_OK(types_.Append(0));
  return nones_.AppendToBitmap(false);
}

Status SequenceBuilder::Append(bool data) {
  NUMBUF_LIST_UPDATE(bools_.length(), BOOL_TAG);
  return bools_.Append(data);
}

Status SequenceBuilder::Append(int64_t data) {
  NUMBUF_LIST_UPDATE(ints_.length(), INT_TAG);
  return ints_.Append(data);
}

Status SequenceBuilder::Append(const char* data) {
  NUMBUF_LIST_UPDATE(strings_.length(), STRING_TAG);
  return strings_.Append(std::string(data, strlen(data)));
}

Status SequenceBuilder::Append(float data) {
  NUMBUF_LIST_UPDATE(floats_.length(), FLOAT_TAG);
  return floats_.Append(data);
}

Status SequenceBuilder::Append(double data) {
  NUMBUF_LIST_UPDATE(doubles_.length(), DOUBLE_TAG);
  return doubles_.Append(data);
}

Status SequenceBuilder::Append(const std::vector<int64_t>& dims, double* data) {
  NUMBUF_LIST_UPDATE(tensors_.length(), TENSOR_TAG);
  return tensors_.Append(dims, data);
}

Status SequenceBuilder::AppendList(int32_t size) {
  NUMBUF_LIST_UPDATE(list_offsets_.size() - 1, LIST_TAG);
  list_offsets_.push_back(list_offsets_.back() + size);
  return Status::OK();
}

Status SequenceBuilder::AppendDict(int32_t size) {
  NUMBUF_LIST_UPDATE(dict_offsets_.size() - 1, DICT_TAG);
  dict_offsets_.push_back(dict_offsets_.back() + size);
  return Status::OK();
}

#define NUMBUF_LIST_ADD(VARNAME, TAG)     \
  types[TAG] = VARNAME.type();            \
  children[TAG] = VARNAME.Finish();       \

std::shared_ptr<DenseUnionArray> SequenceBuilder::Finish(
  std::shared_ptr<Array> list_data,
  std::shared_ptr<Array> dict_data) {

  std::vector<TypePtr> types(NUM_TAGS);
  std::vector<ArrayPtr> children(NUM_TAGS);

  NUMBUF_LIST_ADD(bools_, BOOL_TAG);
  NUMBUF_LIST_ADD(ints_, INT_TAG);
  NUMBUF_LIST_ADD(strings_, STRING_TAG);
  NUMBUF_LIST_ADD(floats_, FLOAT_TAG);
  NUMBUF_LIST_ADD(doubles_, DOUBLE_TAG);
  NUMBUF_LIST_ADD(tensors_, TENSOR_TAG);

  // Finish construction of the lists contained in this list
  list_data = list_data ? list_data : std::make_shared<NullArray>(0);
  DCHECK(list_data->length() == list_offsets_.back());
  ListBuilder list_builder(pool_, list_data);
  ARROW_CHECK_OK(list_builder.Append(list_offsets_.data(), list_offsets_.size()));
  NUMBUF_LIST_ADD(list_builder, LIST_TAG);

  // Finish construction of the dictionaries contained in this list
  dict_data = dict_data ? dict_data : std::make_shared<NullArray>(0);
  DCHECK(dict_data->length() == dict_offsets_.back());
  ListBuilder dict_builder(pool_, dict_data);
  ARROW_CHECK_OK(dict_builder.Append(dict_offsets_.data(), dict_offsets_.size()));
  NUMBUF_LIST_ADD(dict_builder, DICT_TAG);

  TypePtr type = TypePtr(new DenseUnionType(types));

  return std::make_shared<DenseUnionArray>(type, types_.length(),
           children, types_.data(), offsets_.data(),
           nones_.null_count(), nones_.null_bitmap());
}

}
