#include <sys/socket.h>
#include "syscall.h"
#include "libc.h"

int accept(int fd, struct sockaddr* restrict addr, socklen_t* restrict len) {
  return syscall_cp(SYS_accept, fd, addr, len, 0, 0, 0);
}
