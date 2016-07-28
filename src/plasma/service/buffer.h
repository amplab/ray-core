#ifndef PLASMA_BUFFER_H_
#define PLASMA_BUFFER_H_

#include <vector>
#include <memory>
#include "base/logging.h"

namespace plasma {

typedef std::string ObjectID;

typedef void Status;

class PlasmaInterface;

/*! Read-only view on data
*/
class Buffer {
 public:
  // we declare ClientContext friend so it can initialize our private fields
  friend class ClientContext;

  Buffer() : data_(nullptr), size_(0) {}

  Buffer(const uint8_t* data, int64_t size) : data_(data), size_(size) {}
  /*! Return the start address of the buffer.
  */
  const uint8_t* data() { return data_; }
  /*! Return an address corresponding to an "offset" in this buffer
  */
  const uint8_t* data(uint64_t offset) { return data_ + offset; }
  /*! Return the size of the object in bytes
  */
  int64_t size() { return size_; }
  ~Buffer() {}

 private:
  const uint8_t* data_;
  int64_t size_;
};

/*! Mutable view on data
*/
class MutableBuffer : public Buffer {
public:
  // we declare ClientContext friend so it can initialize our private fields
  friend class ClientContext;

  /*! After the default constructor has been called, the class is not
      functional and all methods will raise errors. Only after it has been
      initialized by ClientContext::BuildObject can this class be used.
  */
  MutableBuffer();

  ~MutableBuffer();

  /*! Return the start address of the buffer (mutable).
  */
  uint8_t* mutable_data();
  /*! Return an address corresponding to an "offset" in this buffer (mutable).
  */
  uint8_t* mutable_data(uint64_t offset);
  /*! Resize the buffer.

      \param new_size
        New size of the buffer (in bytes).
  */
  Status Resize(int64_t new_size);
  /*! Make the data contained in this buffer immutable. After the buffer
      has been sealed, it is illegal to modify data from the buffer or to
      resize the buffer.
  */
  Status Seal();
  /*! Has this MutableBuffer been sealed?
  */
  bool sealed() { return sealed_; }

private:
  uint8_t* mutable_data_;
  bool sealed_;
  plasma::ObjectID object_id_;
  std::shared_ptr<PlasmaInterface> interface_;
};

} // namespace plasma

#endif
