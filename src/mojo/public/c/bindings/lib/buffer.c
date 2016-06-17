// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/buffer.h"

#include <assert.h>
#include <stddef.h>  // for NULL

void* MojomBuffer_Allocate(struct MojomBuffer* buf, uint32_t num_bytes) {
  assert(buf);

  static const size_t kAlignment = 8;
  const uint32_t bytes_used = buf->num_bytes_used;

  // size = num_bytes is rounded to 8 bytes:
  const uint64_t size = ((uint64_t)num_bytes + (kAlignment-1))
                        & ~(kAlignment-1);
  if (bytes_used + size > buf->buf_size)
    return NULL;

  buf->num_bytes_used += size;
  return buf->buf + bytes_used;
}
