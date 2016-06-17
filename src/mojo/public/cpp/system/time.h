// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides C++ wrappers for the C API in
// "mojo/public/c/system/time.h".

#ifndef MOJO_PUBLIC_CPP_SYSTEM_TIME_H_
#define MOJO_PUBLIC_CPP_SYSTEM_TIME_H_

#include "mojo/public/c/system/time.h"

namespace mojo {

// Returns the current |MojoTimeTicks| value. See |MojoGetTimeTicksNow()| for
// complete documentation.
inline MojoTimeTicks GetTimeTicksNow() {
  return MojoGetTimeTicksNow();
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_SYSTEM_TIME_H_
