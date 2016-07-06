#include "api.h"

namespace plasma {

PlasmaInterface::PlasmaInterface(const std::string& address,
                                 const std::string& child_connection_id) {
    context_.ConnectToShell(std::string("mojo:plasma"), std::string(address),
                            std::string(child_connection_id));
    interface_ = context_.GetInterface();
}

PlasmaInterface::~PlasmaInterface() {}

mojo::SynchronousInterfacePtr<plasma::service::Plasma>& PlasmaInterface::get() {
  return interface_;
}

ClientContext::ClientContext(const std::string& address,
                             const std::string& child_connection_id) {
  interface_ = std::make_shared<PlasmaInterface>(address, child_connection_id);
}

ClientContext::~ClientContext() {}

Status ClientContext::BuildObject(ObjectID object_id, int64_t size,
                                  MutableBuffer& buffer, const std::string& name) {
  mojo::ScopedSharedBufferHandle handle; // TODO(pcm): Check if we need to hold onto this
  interface_->get()->CreateObject(object_id, size, name, &handle);
  void* pointer = nullptr;
  CHECK_EQ(MOJO_RESULT_OK, mojo::MapBuffer(handle.get(), 0, size, &pointer, MOJO_MAP_BUFFER_FLAG_NONE));
}

}
