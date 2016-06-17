// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_STRUCT_H_
#define MOJO_PUBLIC_C_BINDINGS_STRUCT_H_

#include <stdint.h>

#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

struct MojomStructHeader {
  // num_bytes includes the size of this struct header along with the
  // accompanying struct data.
  uint32_t num_bytes;
  uint32_t version;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomStructHeader) == 8,
                   "struct MojomStructHeader must be 8 bytes.");

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_STRUCT_H_
