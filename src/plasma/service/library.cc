#include "api.h"

#include "plasma_interface.h"

namespace plasma {

ObjectInfo::ObjectInfo() {}

ObjectInfo::~ObjectInfo() {}

ClientContext::ClientContext(const std::string& address) {
  interface_ = std::make_shared<PlasmaInterface>(address);
}

ClientContext::~ClientContext() {}

Status ClientContext::BuildObject(ObjectID object_id, int64_t size,
                                  MutableBuffer& buffer, const std::string& name) {
  mojo::ScopedSharedBufferHandle handle; // TODO(pcm): Check if we need to hold onto this
  interface_->get()->CreateObject(object_id, size, name, &handle);
  void* pointer = nullptr;
  CHECK_EQ(MOJO_RESULT_OK, mojo::MapBuffer(handle.get(), 0, size, &pointer, MOJO_MAP_BUFFER_FLAG_NONE));
  buffer.object_id_ = object_id;
  buffer.mutable_data_ = static_cast<uint8_t*>(pointer);
  buffer.data_ = static_cast<const uint8_t*>(pointer);
  buffer.size_ = size;
  buffer.interface_ = interface_;
}

Status ClientContext::GetObject(ObjectID object_id, Buffer& buffer) {
  mojo::ScopedSharedBufferHandle handle;
  uint64_t size;
  interface_->get()->GetObject(object_id, true, &handle, &size);
  void* pointer = nullptr;
  CHECK_EQ(MOJO_RESULT_OK, mojo::MapBuffer(handle.get(), 0, size, &pointer, MOJO_MAP_BUFFER_FLAG_NONE));
  buffer.data_ = static_cast<const uint8_t*>(pointer);
  buffer.size_ = size;
}

}
