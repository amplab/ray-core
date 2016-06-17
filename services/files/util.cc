// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/files/util.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include <limits>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "mojo/public/cpp/bindings/string.h"

namespace mojo {
namespace files {

Error IsPathValid(const String& path) {
  DCHECK(!path.is_null());
  if (!base::IsStringUTF8(path.get()))
    return Error::INVALID_ARGUMENT;
  if (path.size() > 0 && path[0] == '/')
    return Error::PERMISSION_DENIED;
  return Error::OK;
}

Error IsWhenceValid(Whence whence) {
  return (whence == Whence::FROM_CURRENT || whence == Whence::FROM_START ||
          whence == Whence::FROM_END)
             ? Error::OK
             : Error::UNIMPLEMENTED;
}

Error IsOffsetValid(int64_t offset) {
  return (offset >= std::numeric_limits<off_t>::min() &&
          offset <= std::numeric_limits<off_t>::max())
             ? Error::OK
             : Error::OUT_OF_RANGE;
}

Error ErrnoToError(int errno_value) {
  // TODO(vtl)
  return Error::UNKNOWN;
}

int WhenceToStandardWhence(Whence whence) {
  DCHECK_EQ(IsWhenceValid(whence), Error::OK);
  switch (whence) {
    case Whence::FROM_CURRENT:
      return SEEK_CUR;
    case Whence::FROM_START:
      return SEEK_SET;
    case Whence::FROM_END:
      return SEEK_END;
  }
  NOTREACHED();
  return 0;
}

Error TimespecToStandardTimespec(const Timespec* in, struct timespec* out) {
  if (!in) {
    out->tv_sec = 0;
    out->tv_nsec = UTIME_OMIT;
    return Error::OK;
  }

  static_assert(sizeof(int64_t) >= sizeof(time_t), "whoa, time_t is huge");
  if (in->seconds < std::numeric_limits<time_t>::min() ||
      in->seconds > std::numeric_limits<time_t>::max())
    return Error::OUT_OF_RANGE;

  if (in->nanoseconds < 0 || in->nanoseconds >= 1000000000)
    return Error::INVALID_ARGUMENT;

  out->tv_sec = static_cast<time_t>(in->seconds);
  out->tv_nsec = static_cast<long>(in->nanoseconds);
  return Error::OK;
}

Error TimespecOrNowToStandardTimespec(const TimespecOrNow* in,
                                      struct timespec* out) {
  if (!in) {
    out->tv_sec = 0;
    out->tv_nsec = UTIME_OMIT;
    return Error::OK;
  }

  if (in->now) {
    if (!in->timespec.is_null())
      return Error::INVALID_ARGUMENT;
    out->tv_sec = 0;
    out->tv_nsec = UTIME_NOW;
    return Error::OK;
  }

  return TimespecToStandardTimespec(in->timespec.get(), out);
}

}  // namespace files
}  // namespace mojo
