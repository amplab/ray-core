// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_support/make_pty_pair.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"

namespace native_support {

bool MakePtyPair(base::ScopedFD* master_fd,
                 base::ScopedFD* slave_fd,
                 int* errno_value) {
  DCHECK(master_fd);
  DCHECK(!master_fd->is_valid());  // Not very wrong, but unlikely.
  DCHECK(slave_fd);
  DCHECK(!slave_fd->is_valid());  // Not very wrong, but unlikely.
  DCHECK(errno_value);

#if defined(FNL_MUSL)
  base::ScopedFD master(posix_openpt(O_RDWR | O_NOCTTY));
#else
  // Android doesn't yet have posix_openpt
  base::ScopedFD master(getpt());
#endif
  if (!master.is_valid()) {
    *errno_value = errno;
    PLOG(ERROR) << "posix_openpt/getpt";
    return false;
  }

  if (grantpt(master.get()) < 0) {
    *errno_value = errno;
    PLOG(ERROR) << "grantpt()";
    return false;
  }

  if (unlockpt(master.get()) < 0) {
    *errno_value = errno;
    PLOG(ERROR) << "unlockpt()";
    return false;
  }

  const char* name = ptsname(master.get());
  if (!name) {
    *errno_value = errno;
    PLOG(ERROR) << "ptsname()";
    return false;
  }

  base::ScopedFD slave(HANDLE_EINTR(open(name, O_RDWR)));
  if (!slave.is_valid()) {
    *errno_value = errno;
    PLOG(ERROR) << "open()";
    return false;
  }

  *master_fd = master.Pass();
  *slave_fd = slave.Pass();
  return true;
}

}  // namespace native_support
