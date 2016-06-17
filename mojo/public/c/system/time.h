// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains Mojo system time-related declarations/definitions.
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_SYSTEM_TIME_H_
#define MOJO_PUBLIC_C_SYSTEM_TIME_H_

#include <stdint.h>

#include "mojo/public/c/system/macros.h"

// |MojoTimeTicks|: A time delta, in microseconds, the meaning of which is
// source-dependent.

typedef int64_t MojoTimeTicks;

// |MojoDeadline|: Used to specify deadlines (timeouts), in microseconds (except
// for |MOJO_DEADLINE_INDEFINITE|).
//   |MOJO_DEADLINE_INDEFINITE| - Used to indicate "forever".

typedef uint64_t MojoDeadline;

#define MOJO_DEADLINE_INDEFINITE ((MojoDeadline)-1)

MOJO_BEGIN_EXTERN_C

// |MojoGetTimeTicksNow()|: Returns the time, in microseconds, since some
// undefined point in the past. The values are only meaningful relative to other
// values that were obtained from the same device without an intervening system
// restart. Such values are guaranteed to be monotonically non-decreasing with
// the passage of real time. Although the units are microseconds, the resolution
// of the clock may vary and is typically in the range of ~1-15 ms.
MojoTimeTicks MojoGetTimeTicksNow(void);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_SYSTEM_TIME_H_
