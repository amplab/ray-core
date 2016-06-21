#include "exchange_file_descriptor.h"

#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>

#include <iostream>

#define CHECK(cond, msg) \
  if (!(cond)) { \
    std::cerr << msg << std::endl; \
    exit(1); \
  }

FileDescriptorSender::FileDescriptorSender(const std::string& address) {
  socket_ = socket(PF_UNIX, SOCK_STREAM, 0);
  CHECK(socket_ != -1, "error creating socket");
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, address.c_str(), sizeof(addr.sun_path));
  addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
  unlink(addr.sun_path);
  size_t len = strlen(addr.sun_path) + sizeof(addr.sun_family);
  CHECK(bind(socket_, (struct sockaddr *)&addr, len) != -1, "error binding socket");
  CHECK(listen(socket_, 5) != -1, "error listening on socket");
}

FileDescriptorSender::~FileDescriptorSender() {
  close(socket_);
}

static void init_msg(struct msghdr *msg, struct iovec *iov, char *buf, size_t buf_len) {
  iov->iov_base = buf;
  iov->iov_len = 1;

  msg->msg_iov = iov;
  msg->msg_iovlen = 1;
  msg->msg_control = buf;
  msg->msg_controllen = buf_len;
  msg->msg_name = NULL;
  msg->msg_namelen = 0;
}

bool FileDescriptorSender::Send(int file_descriptor) {
  struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	int s = accept(socket_, (struct sockaddr *)&addr, &len);

  struct msghdr msg;
  struct iovec iov;
  char buf[CMSG_SPACE(sizeof(int))];

  init_msg(&msg, &iov, buf, sizeof(buf));

  struct cmsghdr *header = CMSG_FIRSTHDR(&msg);
  header->cmsg_level = SOL_SOCKET;
  header->cmsg_type = SCM_RIGHTS;
  header->cmsg_len = CMSG_LEN(sizeof(int));
  *(int *)CMSG_DATA(header) = file_descriptor;

  return sendmsg(s, &msg, 0);
}

FileDescriptorReceiver::FileDescriptorReceiver(const std::string& address) {
  socket_ = socket(PF_UNIX, SOCK_STREAM, 0);
  CHECK(socket_ != -1, "error creating socket");
  struct sockaddr_un addr;
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, address.c_str(), sizeof(addr.sun_path));
  addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
  int r = connect(socket_, (struct sockaddr *)&addr, sizeof(addr));
  CHECK(r != -1, "error connecting to socket");
}

FileDescriptorReceiver::~FileDescriptorReceiver() {
  close(socket_);
}

int FileDescriptorReceiver::Receive() {
  struct msghdr msg;
  struct iovec iov;
  char buf[CMSG_SPACE(sizeof(int))];
  init_msg(&msg, &iov, buf, sizeof(buf));

  if (recvmsg(socket_, &msg, 0) == -1)
    return -1;

  int found_fd = -1;
  bool oh_noes = false;
  for (struct cmsghdr *header = CMSG_FIRSTHDR(&msg); header != NULL; header = CMSG_NXTHDR(&msg, header))
    if (header->cmsg_level == SOL_SOCKET && header->cmsg_type == SCM_RIGHTS) {
      int count = (header->cmsg_len - (CMSG_DATA(header) - (unsigned char *)header)) / sizeof(int);
      for (int i = 0; i < count; ++i) {
        int fd = ((int *)CMSG_DATA(header))[i];
        if (found_fd == -1) {
          found_fd = fd;
        } else {
          close(fd);
          oh_noes = true;
        }
      }
    }

  // The sender sent us more than one file descriptor. We've closed
  // them all to prevent fd leaks but notify the caller that we got
  // a bad message.
  if (oh_noes) {
    close(found_fd);
    errno = EBADMSG;
    return -1;
  }

  return found_fd;
}
