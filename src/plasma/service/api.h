#ifndef PLASMA_API_H_
#define PLASMA_API_H_

#include <map>
#include <string>
#include "buffer.h"

namespace plasma {

typedef int64_t ClientID;

class PlasmaInterface;

class ObjectInfo {
public:
  ObjectInfo();
  //! Name of the object as provided by the user during object construction
  std::string name;
  //! Size of the object in bytes
  int64_t size;
  //! Time when object construction started, in microseconds since the Unix epoch
  int64_t create_time;
  //! Time in microseconds between object creation and sealing
  int64_t construct_duration;
  //! Process ID of the process that created the object
  int64_t creator_id;
  //! Cluster wide unique address for the process that created the object
  std::string creator_address;
};

/*! A client context is the primary interface through which clients interact
    with Plasma.
*/
class ClientContext {
 public:
  /*! Create a new client context.

      \param address
        Adress of the Ray shell socket we are connecting to
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

      \param metadata
        An optional dictionary of metadata for the object. The keys of
        the dictionary are strings and the values are arbitrary binary data
        represented by Buffer objects.
  */
  Status BuildObject(ObjectID object_id, int64_t size,
                     MutableBuffer& buffer, const std::string& name = "",
                     const std::map<std::string, Buffer>& metadata =
                       std::map<std::string, Buffer>());

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
