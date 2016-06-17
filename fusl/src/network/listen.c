#include <sys/socket.h>
#include "syscall.h"

int listen(int fd, int backlog) {
  return syscall(SYS_listen, fd, backlog, 0, 0, 0, 0);
}
