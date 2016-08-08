#ifndef RAYCLIENT_EXCHANGE_FILE_DESCRIPTOR_H
#define RAYCLIENT_EXCHANGE_FILE_DESCRIPTOR_H

#include <string>

namespace ray {

/*! Send a file descriptor of a process to another process. This is needed
    because Mojo bootstraps the IPC communication between processes via a
    file handle (this makes sure no artifacts like actual files remain on the
    computer once the IPC has finished).
*/
class FileDescriptorSender {
public:
  /*! Initialize the FileDescriptorSender.

      \param address
        Address of the socket that is used to send the file descriptor
  */
  FileDescriptorSender(const std::string& address);
  ~FileDescriptorSender();

  /*! Send the file descriptor over the socket.

      \param file_descriptor
        The file descriptor that will be sent

      \param payload
        Additional payload that can be sent (< MAX_PAYLOAD_SIZE)

      \return
        Bool that indicates if the sending was successful
  */
  bool Send(int file_descriptor, const std::string& payload);
private:
  int socket_;
};

/*! Receive a file descriptor from another process. This is needed
    because Mojo bootstraps the IPC communication between processes via a
    file handle (to make sure no artifacts like actual files remain on the
    computer once the IPC has finished).
*/
class FileDescriptorReceiver {
public:
  FileDescriptorReceiver(const std::string& address);
  ~FileDescriptorReceiver();

  /*! Receive file descriptor from the socket.

      \param payload
        The payload carried by this receive will be appended to this string

      \return
        The file descriptor that was sent or -1 if not successful.
  */
  int Receive(std::string& payload);
private:
  int socket_;
};

} // namespace ray

#endif
