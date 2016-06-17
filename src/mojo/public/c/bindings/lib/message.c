// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/message.h"

#include <stdint.h>

#include "mojo/public/c/bindings/struct.h"

MojomValidationResult MojomMessage_ValidateHeader(const void* in_buf,
                                                  uint32_t in_buf_size) {
  const struct MojomStructHeader* header =
    (const struct MojomStructHeader*)in_buf;
  if (in_buf_size < sizeof(struct MojomStructHeader) ||
      in_buf_size < header->num_bytes) {
    return MOJOM_VALIDATION_UNEXPECTED_STRUCT_HEADER;
  }

  const struct MojomMessage* msg = (const struct MojomMessage*)in_buf;
  if (header->version == 0u) {
    if (header->num_bytes != sizeof(struct MojomMessage)) {
      return MOJOM_VALIDATION_UNEXPECTED_STRUCT_HEADER;
    }

    // Version 0 has no request id and should not have either of these flags.
    if ((msg->flags & MOJOM_MESSAGE_FLAGS_EXPECTS_RESPONSE) ||
        (msg->flags & MOJOM_MESSAGE_FLAGS_IS_RESPONSE)) {
      return MOJOM_VALIDATION_MESSAGE_HEADER_MISSING_REQUEST_ID;
    }
  } else if (header->version == 1u) {
    if (header->num_bytes != sizeof(struct MojomMessageWithRequestId)) {
      return MOJOM_VALIDATION_UNEXPECTED_STRUCT_HEADER;
    }
  } else if (header->version > 1u) {
    if (header->num_bytes < sizeof(struct MojomMessageWithRequestId)) {
      return MOJOM_VALIDATION_UNEXPECTED_STRUCT_HEADER;
    }
  }

  // Mutually exclusive flags.
  if ((msg->flags & MOJOM_MESSAGE_FLAGS_EXPECTS_RESPONSE) &&
      (msg->flags & MOJOM_MESSAGE_FLAGS_IS_RESPONSE)) {
    return MOJOM_VALIDATION_MESSAGE_HEADER_INVALID_FLAGS;
  }

  // Accept unknown versions of the message header to be future-proof.
  return MOJOM_VALIDATION_ERROR_NONE;
}
