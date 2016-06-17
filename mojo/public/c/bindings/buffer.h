// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_BUFFER_H_
#define MOJO_PUBLIC_C_BINDINGS_BUFFER_H_

#include <stdint.h>

#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

// |MojomBuffer| is used to track a buffer state for mojom serialization. The
// user must initialize this struct themselves. See the fields for details.
struct MojomBuffer {
  char* buf;
  // The number of bytes described by |buf|.
  uint32_t buf_size;
  // Must be initialized to 0. MojomBuffer_Allocate() will update it as it
  // consumes |buf|.
  uint32_t num_bytes_used;
};

// Allocates |num_bytes| (rounded to 8 bytes) from |buf|. Returns NULL if
// there isn't enough space left to allocate.
void* MojomBuffer_Allocate(struct MojomBuffer* buf, uint32_t num_bytes);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_BUFFER_H_
