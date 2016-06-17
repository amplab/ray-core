#include <sys/socket.h>
#include "syscall.h"

int getsockname(int fd,
                struct sockaddr* restrict addr,
                socklen_t* restrict len) {
  return syscall(SYS_getsockname, fd, addr, len, 0, 0, 0);
}
