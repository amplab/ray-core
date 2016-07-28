#include "api.h"

#include "base/process/process.h"
#include "plasma_interface.h"

namespace plasma {

ObjectInfo::ObjectInfo() {}

ClientContext::ClientContext(const std::string& address) {
  interface_ = std::make_shared<PlasmaInterface>(address);
}

ClientContext::~ClientContext() {}

Status ClientContext::BuildObject(
    ObjectID object_id, int64_t size,
    MutableBuffer& buffer, const std::string& name,
    const std::map<std::string, Buffer>& metadata) {
  mojo::ScopedSharedBufferHandle handle; // TODO(pcm): Check if we need to hold onto this
  int64_t creator_id = base::Process::Current().Pid();
  interface_->get()->CreateObject(object_id, size, name, creator_id, &handle);
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

Status ClientContext::ListObjects(std::vector<ObjectInfo>* objects) {
  mojo::Array<service::ObjectInfoPtr> infos;
  interface_->get()->ListObjects(&infos);
  for (size_t i = 0; i < infos.size(); ++i) {
    ObjectInfo info;
    info.name = infos[i]->name.get();
    info.size = infos[i]->size;
    info.create_time = infos[i]->create_time;
    info.construct_duration = infos[i]->construct_duration;
    info.creator_id = infos[i]->creator_id;
    objects->push_back(info);
  }
}

}
