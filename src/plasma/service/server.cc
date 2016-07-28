#include <unordered_map>
#include "base/time/time.h"
#include "plasma/service/plasma.mojom.h"
#include "mojo/common/binding_set.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/macros.h"

#include "api.h"

using plasma::service::Plasma;
using plasma::ObjectID;

namespace plasma {

namespace service {

/*! An entry in the hash table of objects stored in the local object store.
*/
class PlasmaEntry {
public:
  //! Handle to the shared memory buffer where the object is stored
  mojo::ScopedSharedBufferHandle handle;
  //! ObjectInfo (see plasma.mojom)
  ObjectInfoPtr object_info;
};

/*! Implementation of the Plasma service interface. This implementation is
    single threaded, which means we do not have to lock the datastructures.
*/
class PlasmaImpl : public Plasma {
 public:
  /*! Creates a new object..

      \param object_id
        Unique identifier of the object to be build

      \param size
        Initial number of bytes to be allocated for the object

      \param name
        User defined name of the object

      \return
        Shared memory handle to the read-write memory of the object
  */
  void CreateObject(const mojo::String& object_id, uint64 size, const mojo::String& name, int64 creator_id,
                   const CreateObjectCallback& callback) override {
    mojo::ScopedSharedBufferHandle handle;
    CHECK_EQ(MOJO_RESULT_OK, mojo::CreateSharedBuffer(nullptr, size, &handle));
    DCHECK(handle.is_valid());
    mojo::ScopedSharedBufferHandle handle_copy;
    mojo::DuplicateBuffer(handle.get(), nullptr, &handle_copy);
    DCHECK(handle_copy.is_valid());
    // Set object info
    auto object_info = ObjectInfo::New();
    object_info->name = std::string(name.get());
    object_info->size = size;
    object_info->create_time = base::TimeTicks::Now().ToInternalValue(); // TODO(pcm): Check this
    object_info->construct_duration = -1;
    object_info->creator_id = creator_id;
    open_objects_.emplace(object_id, PlasmaEntry {handle.Pass(), object_info.Pass()});
    callback.Run(handle_copy.Pass());
  }

  void ResizeObject(const mojo::String& object_id, uint64 new_size,
                    const ResizeObjectCallback& callback) override {
    mojo::ScopedSharedBufferHandle handle;
    CHECK_EQ(MOJO_RESULT_OK, mojo::CreateSharedBuffer(nullptr, new_size, &handle));
    CHECK(false);
  }

  /*! Pass a sealed object to a client that has been waiting.
  */
  void pass_sealed_object(const mojo::String& object_id, const GetObjectCallback& callback) {
    mojo::ScopedSharedBufferHandle handle;
    const PlasmaEntry& object = sealed_objects_[object_id];
    mojo::DuplicateBuffer(object.handle.get(), nullptr, &handle);
    DCHECK(handle.is_valid());
    callback.Run(handle.Pass(), object.object_info->size);
  }

  /*! Seal an object, making it immutable.

      \param object_id
        Unique identifier of the object to be sealed
  */
  void SealObject(const mojo::String& object_id) override {
    // TODO(pcm): Check this
    open_objects_[object_id].object_info->construct_duration =
      base::TimeTicks::Now().ToInternalValue() -
        open_objects_[object_id].object_info->create_time;
    sealed_objects_[object_id] = std::move(open_objects_[object_id]);
    open_objects_.erase(object_id);
    for (auto elem : objects_notify_[object_id]) {
      pass_sealed_object(object_id, elem);
    }
    objects_notify_[object_id].clear();
  }

  /*! Get an object from the object store.

      \param object_id
        Unique identifier of the object that shall be returned

      \param block
        If true, this call will block until the object becomes available.
        Otherwise, if the object is not in the object store, an error will
        be raised.

      \return
        Handle to the object and size of the object in bytes
  */
  void GetObject(const mojo::String& object_id, bool block,
                 const GetObjectCallback& callback) override {
    auto entry = sealed_objects_.find(object_id);
    if (entry == sealed_objects_.end()) {
      objects_notify_[object_id].push_back(callback);
    } else {
      pass_sealed_object(object_id, callback);
    }
  }

  /*! List objects from the object store.

      \return
        A list of ObjectInfoData objects that describe the objects in the store.
  */
  void ListObjects(const ListObjectsCallback& callback) override {
    auto object_info = mojo::Array<ObjectInfoPtr>::New(0);
    for (const auto & entry : sealed_objects_) {
      object_info.push_back(entry.second.object_info->Clone());
    }
    for (const auto & entry : open_objects_) {
      object_info.push_back(entry.second.object_info->Clone());
    }
    callback.Run(object_info.Pass());
  }

 private:
  //! Hash table of objects that have already been sealed
  std::unordered_map<ObjectID, PlasmaEntry> sealed_objects_;
  //! Hash table of objects that are under construction
  std::unordered_map<ObjectID, PlasmaEntry> open_objects_;
  //! Requests for objects that have not been sealed yet. For each object,
  //! we store a list of callbacks that will be used to pass the object
  //! to the client, one for each client waiting for the object.
  std::unordered_map<ObjectID, std::vector<GetObjectCallback>> objects_notify_;
};

/*! Implementation of the Plasma server. This follows the "SingletonServer"
    pattern in examples/echo/echo_server.cc (see documentation there).
    This means that the object store is shared between all the clients
    running on a given node.
*/
class PlasmaServerApp : public mojo::ApplicationImplBase {
 public:
  PlasmaServerApp() {}
  ~PlasmaServerApp() override {}

  /*! Accept a new connection from a client.
  */
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<Plasma>(
        [this](const mojo::ConnectionContext& connection_context,
           mojo::InterfaceRequest<Plasma> plasma_request) {
          bindings_.AddBinding(&plasma_impl_, plasma_request.Pass());
        });
    return true;
  }

 private:
  PlasmaImpl plasma_impl_;

  mojo::BindingSet<Plasma> bindings_;
};

} // namespace service

}  // namespace plasma

MojoResult MojoMain(MojoHandle application_request) {
  plasma::service::PlasmaServerApp plasma_server_app;
  return mojo::RunApplication(application_request, &plasma_server_app);
}
