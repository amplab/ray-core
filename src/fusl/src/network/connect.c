#include <sys/socket.h>
#include "syscall.h"
#include "libc.h"

int connect(int fd, const struct sockaddr* addr, socklen_t len) {
  return syscall_cp(SYS_connect, fd, addr, len, 0, 0, 0);
}
