// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_MESSAGE_H_
#define MOJO_PUBLIC_C_BINDINGS_MESSAGE_H_

#include <stdint.h>

#include "mojo/public/c/bindings/struct.h"
#include "mojo/public/c/bindings/validation.h"
#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

#define MOJOM_MESSAGE_FLAGS_EXPECTS_RESPONSE ((uint32_t)(1 << 0u))
#define MOJOM_MESSAGE_FLAGS_IS_RESPONSE ((uint32_t)(1 << 1u))

// All mojom messages (over a message pipe) are framed with a MojomMessage as
// its header.
// MojomMessage is actually a mojom struct -- we define it here since it's
// easier to read, and the validation has more to it than simple mojom struct
// validation.
struct MojomMessage {
  // header.version = 0 (version 1 has a request_id)
  struct MojomStructHeader header;
  // The ordinal number associated with this message. This is specified
  // (implicitly, if not explicitly) in the mojom IDL for an interface message
  // and is used to identify it.
  uint32_t ordinal;
  // Described by the MOJOM_MESSAGE_* flags defined above.
  uint32_t flags;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomMessage) == 16,
                   "struct MojomMessage must be 16 bytes");

// Using |MojomMessage| as a member of this struct could work, but usability
// would suffer a little while accessing the |header|. MojomMessageWithRequestId
// is a "newer" version than MojomMessage.
struct MojomMessageWithRequestId {
  // header.version = 1
  struct MojomStructHeader header;
  // Which message is it?
  uint32_t ordinal;
  // Described by the MOJOM_MESSAGE_* flags defined above.
  uint32_t flags;
  uint64_t request_id;
};
MOJO_STATIC_ASSERT(sizeof(struct MojomMessageWithRequestId) == 24,
                   "MojomMessageWithRequestId must be 24 bytes");

// Validates that |in_buf| is a valid mojom message. This does not validate the
// contents of the message's body, only the message header.
// |in_buf|: can be a MojomMessage or a MojomMessageWithRequestId.
// |in_buf_size|: number of bytes in |in_buf|.
MojomValidationResult MojomMessage_ValidateHeader(const void* in_buf,
                                                  uint32_t in_buf_size);

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_MESSAGE_H_
