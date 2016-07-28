#include "exchange_file_descriptor.h"

#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#include "base/logging.h"
#include "base/posix/unix_domain_socket_linux.h"

namespace ray {

const size_t MAX_PAYLOAD_SIZE = 1024;

FileDescriptorSender::FileDescriptorSender(const std::string& address) {
  socket_ = socket(PF_UNIX, SOCK_STREAM, 0);
  CHECK(socket_ != -1) << "error creating socket: " << strerror(errno);
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, address.c_str(), sizeof(addr.sun_path));
  addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
  unlink(addr.sun_path);
  size_t len = strlen(addr.sun_path) + sizeof(addr.sun_family);
  int status = bind(socket_, (struct sockaddr *)&addr, len);
  CHECK(status != -1) << "error binding socket: " << strerror(errno);
  status = listen(socket_, 5);
  CHECK(status != -1) << "error listening on socket: " << strerror(errno);
}

FileDescriptorSender::~FileDescriptorSender() {
  close(socket_);
}

bool FileDescriptorSender::Send(int fd, const std::string& payload) {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int s = accept(socket_, (struct sockaddr *)&addr, &len);
  CHECK(s != -1) << "error accepting incoming connection: " << strerror(errno);
  ssize_t res = base::UnixDomainSocket::SendMsg(s, payload.c_str(),
      payload.size(), std::vector<int>({fd}));
  return res != -1;
}

FileDescriptorReceiver::FileDescriptorReceiver(const std::string& address) {
  socket_ = socket(PF_UNIX, SOCK_STREAM, 0);
  CHECK(socket_ != -1) << "error creating socket: " << strerror(errno);
  struct sockaddr_un addr;
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, address.c_str(), sizeof(addr.sun_path));
  addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
  int r = connect(socket_, (struct sockaddr *)&addr, sizeof(addr));
  CHECK(r != -1) << "error connecting to socket: " << strerror(errno);
}

FileDescriptorReceiver::~FileDescriptorReceiver() {
  close(socket_);
}

int FileDescriptorReceiver::Receive(std::string& payload) {
  ScopedVector<base::ScopedFD> fds;
  char reply[MAX_PAYLOAD_SIZE];
  ssize_t len = base::UnixDomainSocket::RecvMsg(socket_,
      reply, MAX_PAYLOAD_SIZE, &fds);
  CHECK(len != -1) << "file descriptor could not be received";
  payload += std::string(reply, len);
  return fds[0]->release();
}

} // namespace ray
