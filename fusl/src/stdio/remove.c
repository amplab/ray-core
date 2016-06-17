#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "syscall.h"

int remove(const char* path) {
  int r = unlink(path);
#ifdef SYS_rmdir
  if (r == -EISDIR)
    r = __syscall(SYS_rmdir, path);
#else
  if (r == -EISDIR)
    r = __syscall(SYS_unlinkat, AT_FDCWD, path, AT_REMOVEDIR);
#endif
  return __syscall_ret(r);
}
