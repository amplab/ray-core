// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_C_BINDINGS_VALIDATION_H_
#define MOJO_PUBLIC_C_BINDINGS_VALIDATION_H_

#include <stdint.h>

#include "mojo/public/c/system/macros.h"

MOJO_BEGIN_EXTERN_C

// One of MOJOM_VALIDATION_* codes;  See below.
typedef uint32_t MojomValidationResult;

// No error; successful validation.
#define MOJOM_VALIDATION_ERROR_NONE ((MojomValidationResult)0)
// An object (struct or array) is not 8-byte aligned.
#define MOJOM_VALIDATION_MISALIGNED_OBJECT ((MojomValidationResult)1)
// An object is not contained inside the message data, or it overlaps other
// objects.
#define MOJOM_VALIDATION_ILLEGAL_MEMORY_RANGE ((MojomValidationResult)2)
// A struct header doesn't make sense, for example:
// - |num_bytes| is smaller than the size of the struct header.
// - |num_bytes| and |version| don't match.
#define MOJOM_VALIDATION_UNEXPECTED_STRUCT_HEADER ((MojomValidationResult)3)
// An array header doesn't make sense, for example:
// - |num_bytes| is smaller than the size of the header plus the size required
// to store |num_elements| elements.
// - For fixed-size arrays, |num_elements| is different than the specified
// size.
#define MOJOM_VALIDATION_UNEXPECTED_ARRAY_HEADER ((MojomValidationResult)4)
// An encoded handle is illegal.
#define MOJOM_VALIDATION_ILLEGAL_HANDLE ((MojomValidationResult)5)
// A non-nullable handle field is set to invalid handle.
#define MOJOM_VALIDATION_UNEXPECTED_INVALID_HANDLE ((MojomValidationResult)6)
// An encoded pointer is illegal.
#define MOJOM_VALIDATION_ILLEGAL_POINTER ((MojomValidationResult)7)
// A non-nullable pointer field is set to null.
#define MOJOM_VALIDATION_UNEXPECTED_NULL_POINTER ((MojomValidationResult)8)
// |flags| in the message header is invalid. The flags are either
// inconsistent with one another, inconsistent with other parts of the
// message, or unexpected for the message receiver.  For example the
// receiver is expecting a request message but the flags indicate that
// the message is a response message.
#define MOJOM_VALIDATION_MESSAGE_HEADER_INVALID_FLAGS ((MojomValidationResult)9)
// |flags| in the message header indicates that a request ID is required but
// there isn't one.
#define MOJOM_VALIDATION_MESSAGE_HEADER_MISSING_REQUEST_ID \
  ((MojomValidationResult)10)
// The |name| field in a message header contains an unexpected value.
#define MOJOM_VALIDATION_MESSAGE_HEADER_UNKNOWN_METHOD \
  ((MojomValidationResult)11)
// Two parallel arrays which are supposed to represent a map have different
// lengths.
#define MOJOM_VALIDATION_DIFFERENT_SIZED_ARRAYS_IN_MAP \
  ((MojomValidationResult)12)
// A non-nullable union is set to null. (Has size 0)
#define MOJOM_VALIDATION_UNEXPECTED_NULL_UNION ((MojomValidationResult)13)

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_BINDINGS_VALIDATION_H_
