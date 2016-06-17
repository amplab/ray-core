#include <sys/socket.h>
#include "syscall.h"
#include "libc.h"

ssize_t sendto(int fd,
               const void* buf,
               size_t len,
               int flags,
               const struct sockaddr* addr,
               socklen_t alen) {
  return syscall_cp(SYS_sendto, fd, buf, len, flags, addr, alen);
}
