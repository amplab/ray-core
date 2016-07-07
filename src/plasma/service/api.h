#ifndef PLASMA_API_H_
#define PLASMA_API_H_

#include <string>
#include "buffer.h"

namespace plasma {

typedef int64_t ClientID;

class PlasmaInterface;

class ObjectInfo {
 public:
  ObjectInfo();
  ~ObjectInfo();
  std::string name() const;
  int64_t size() const;
  int64_t creation_time() const;
  int64_t sealing_time() const;
  uint64_t creator() const;
};

/*! A client context is the primary interface through which clients interact
    with Plasma.
*/
class ClientContext {
 public:
  /*! Create a new client context.

      \param (client_id)
        Unique identifier of the client that connects to Plasma. This client
        will be owner over all objects created through this context.

      \param address
        Adress of the Ray shell socket we are connecting to

      \param child_connection_id
        Unique identifier of this connection (allocated by the Ray shell)
  */
  ClientContext(const std::string& address);

  ~ClientContext();

  /*! Build a new object. Building an object involves multiple steps.
      Once the creator process finishes to construct the objects, it
      seals the object. Only after that can it be shared with other
      processes.

      \param object_id
        The object ID of the newly created objects. Provided by the
        client, which must ensure it is globally unique.

      \param size
        The number of bytes that are allocated for the object
        initially. Can be reallocated through the MutableBuffer
        interface.

      \param buffer
        The function will pass the allocated buffer to the client
        using this argument.

      \param name
        An optional name for the object through which is can be
        accessed without knowing its object ID.
  */
  Status BuildObject(ObjectID object_id, int64_t size,
                     MutableBuffer& buffer, const std::string& name = "");

  /*! Get buffer associated to an object ID. If the object has not
	    been sealed yet, this function will block the current thread.

      \param object_id
        The object ID of the object that shall be retrieved.

      \param buffer
        The argument is used to pass the read-only buffer to the client.
  */
  Status GetObject(ObjectID object_id, Buffer& buffer);

  /*! Put object information of objects in the store into the
      vector objects.
  */
  Status ListObjects(std::vector<ObjectInfo>* objects);

  /*! Store user provided metadata for a given object. Can only be set by the
      creator of the object an only before the object is sealed.
  */
  Status SetMetadata(ObjectID object_id, const std::string& key, Buffer data);

  /*! Retrieve metadata for a given object.

      \param key
        The key of the metadata information to be retrieved.

      \return
        A view on the metadata associated to that key.
  */
  Status GetMetadata(ObjectID object_id, const std::string& key, Buffer& data);

 private:
  std::shared_ptr<PlasmaInterface> interface_;
};

}

#endif
