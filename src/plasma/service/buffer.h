#include <vector>
#include <memory>
#include "base/logging.h"

namespace plasma {

typedef void Status;

/*! Read-only view on data
*/
class Buffer {
 public:
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
  // Opaque pointer to break dependency chain
  class PlasmaInterface;
  std::shared_ptr<PlasmaInterface> interface_;
};

} // namespace plasma
