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
      list_offsets_({0}), tuple_offsets_({0}), dict_offsets_({0}) {}

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

Status SequenceBuilder::AppendTuple(int32_t size) {
  NUMBUF_LIST_UPDATE(tuple_offsets_.size() - 1, TUPLE_TAG);
  tuple_offsets_.push_back(tuple_offsets_.back() + size);
  return Status::OK();
}

Status SequenceBuilder::AppendDict(int32_t size) {
  NUMBUF_LIST_UPDATE(dict_offsets_.size() - 1, DICT_TAG);
  dict_offsets_.push_back(dict_offsets_.back() + size);
  return Status::OK();
}

#define SEQUENCE_ADD_ELEMENT(VARNAME, TAG)       \
  types[TAG] = VARNAME.type();                   \
  children[TAG] = VARNAME.Finish();              \

#define SEQUENCE_ADD_SUBSEQUENCE(DATA, OFFSETS, BUILDER, TAG)       \
  DATA = DATA ? DATA : std::make_shared<NullArray>(0);              \
  DCHECK(DATA->length() == OFFSETS.back());                         \
  ListBuilder BUILDER(pool_, DATA);                                 \
  ARROW_CHECK_OK(BUILDER.Append(OFFSETS.data(), OFFSETS.size()));   \
  SEQUENCE_ADD_ELEMENT(BUILDER, TAG);

std::shared_ptr<DenseUnionArray> SequenceBuilder::Finish(
  std::shared_ptr<Array> list_data,
  std::shared_ptr<Array> tuple_data,
  std::shared_ptr<Array> dict_data) {

  std::vector<TypePtr> types(NUM_TAGS);
  std::vector<ArrayPtr> children(NUM_TAGS);

  SEQUENCE_ADD_ELEMENT(bools_, BOOL_TAG);
  SEQUENCE_ADD_ELEMENT(ints_, INT_TAG);
  SEQUENCE_ADD_ELEMENT(strings_, STRING_TAG);
  SEQUENCE_ADD_ELEMENT(floats_, FLOAT_TAG);
  SEQUENCE_ADD_ELEMENT(doubles_, DOUBLE_TAG);
  SEQUENCE_ADD_ELEMENT(tensors_, TENSOR_TAG);

  SEQUENCE_ADD_SUBSEQUENCE(list_data, list_offsets_, list_builder, LIST_TAG);
  SEQUENCE_ADD_SUBSEQUENCE(tuple_data, tuple_offsets_, tuple_builder, TUPLE_TAG);
  SEQUENCE_ADD_SUBSEQUENCE(dict_data, dict_offsets_, dict_builder, DICT_TAG);

  TypePtr type = TypePtr(new DenseUnionType(types));

  return std::make_shared<DenseUnionArray>(type, types_.length(),
           children, types_.data(), offsets_.data(),
           nones_.null_count(), nones_.null_bitmap());
}

}
