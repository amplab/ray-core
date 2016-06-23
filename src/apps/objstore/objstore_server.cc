#include "apps/objstore/objstore.mojom.h"
#include "mojo/common/binding_set.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/macros.h"

using mojo::apps::ObjStore;

namespace {

class ObjStoreImpl : public ObjStore {
 public:
  explicit ObjStoreImpl(mojo::InterfaceRequest<ObjStore> objstore_request)
      : strong_binding_(this, std::move(objstore_request)) {}
  ~ObjStoreImpl() override {}

  void BuildObject(int64 object_id, int64 size,
                   const mojo::Callback<void(mojo::ScopedSharedBufferHandle)>& callback) override {
    MOJO_CHECK(size >= 0);
    mojo::ScopedSharedBufferHandle handle;
    assert(MOJO_RESULT_OK == mojo::CreateSharedBuffer(nullptr, size, &handle));
    mojo::ScopedSharedBufferHandle handle_copy;
    mojo::DuplicateBuffer(handle.get(), nullptr, &handle_copy);
    memory_handles_.push_back(std::move(handle));
    callback.Run(std::move(handle_copy));
  }

 private:
  mojo::StrongBinding<ObjStore> strong_binding_;
  std::vector<mojo::ScopedSharedBufferHandle> memory_handles_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ObjStoreImpl);
};

class ObjStoreServerApp : public mojo::ApplicationImplBase {
 public:
  ObjStoreServerApp() {}
  ~ObjStoreServerApp() override {}

  // |mojo::ApplicationImplBase| override:
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<ObjStore>(
        [](const mojo::ConnectionContext& connection_context,
           mojo::InterfaceRequest<ObjStore> objstore_request) {
          new ObjStoreImpl(std::move(objstore_request));  // Owns itself.
        });
    return true;
  }

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(ObjStoreServerApp);
};

}  // namespace

MojoResult MojoMain(MojoHandle application_request) {
  ObjStoreServerApp objstore_server_app;
  return mojo::RunApplication(application_request, &objstore_server_app);
}
