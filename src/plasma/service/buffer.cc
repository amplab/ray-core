#include "buffer.h"

namespace plasma {

MutableBuffer::MutableBuffer() : Buffer(nullptr, 0), mutable_data_(nullptr) {}

MutableBuffer::~MutableBuffer() {
  CHECK(sealed_) << "MutableBuffer must be sealed befor it goes out of scope";
}

uint8_t* MutableBuffer::mutable_data() {
  DCHECK(mutable_data_);
  DCHECK(!sealed_);
  return mutable_data_;
}

Status MutableBuffer::Resize(int64_t new_size) {
  DCHECK(interface_);
  CHECK(false);
}

} // namespace plasma
