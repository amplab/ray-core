// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the definition of |MojoResult| and related macros.
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_SYSTEM_RESULT_H_
#define MOJO_PUBLIC_C_SYSTEM_RESULT_H_

#include <stdint.h>

// |MojoResult|: A type to hold error codes, which are composed of a generic
// portion (the base error code) and a more specific portion (the error subcode
// and the error space).
//
// The format of a |MojoResult| is, bitwise (least significant bits on the
// right):
//
//     sSSSSSSSSSSSSSSSeeeeeeeeeeeeEEEE
//
// where:
//   - EEEE is the base error code (4 bits, 0-15);
//   - eeeeeeeeeeee is the error subcode (12 bits, 0-4095); and
//   - sSSSSSSSSSSSSSSS (16 bits, 0-65535) is the error space.
//
// The meaning of the base error codes is always the same. |MOJO_ERROR_CODE_OK|,
// 0, always means "OK"; for this case, both the error subcode and error space
// must also be 0. See the definitions of the |MOJO_ERROR_CODE_...| below for
// information on the other base error codes.
//
// The meaning of the error subcode depends on both the error space and also the
// base error code, though |MOJO_ERROR_SUBCODE_GENERIC|, 0, always means
// "generic"/"unspecified".
//
// The error space gives meaning to error subcodes:
//   - Error spaces with the high ('s') bit set (i.e., greater than or equal to
//     32768) are reserved for private use, i.e., should not be used in public
//     interfaces.
//   - Error spaces without the high bit set are assigned "centrally", with
//     error subcodes within them assigned by their "owners". (TODO(vtl): Figure
//     out what all of this means.)
//   - |MOJO_ERROR_SPACE_SYSTEM|, 0, is the system/generic error space. Error
//     subcodes for this error space are assigned below.

typedef uint32_t MojoResult;

// |MOJO_RESULT_MAKE()|: Helper macro to make a |MojoResult| from the base error
// code, the error space, and the error subcode.
#define MOJO_RESULT_MAKE(base_error_code, error_space, error_subcode) \
  ((base_error_code) | ((error_space) << 16u) | ((error_subcode) << 4u))

// |MOJO_RESULT_GET_{CODE,SPACE,SUBCODE}()|: Helper macros to get the base error
// code, the error space, and the error subcode from a |MojoResult|.
#define MOJO_RESULT_GET_CODE(result) ((result) & ((MojoResult)0xfu))
#define MOJO_RESULT_GET_SPACE(result) \
  (((result) & ((MojoResult)0xffff0000u)) >> 16u)
#define MOJO_RESULT_GET_SUBCODE(result) \
  (((result) & ((MojoResult)0xfff0u)) >> 4u)

// Base error codes ------------------------------------------------------------

// The base error code indicates the general type of error (as described below).
// For non-success codes, additional information may be conveyed via the error
// space and error subcodes (for |MOJO_ERROR_CODE_OK|, the error space and error
// subcode must be zero):
//   |MOJO_ERROR_CODE_OK| - Not an error; returned on success.
//   |MOJO_ERROR_CODE_CANCELLED| - Operation was cancelled, typically by the
//       caller.
//   |MOJO_ERROR_CODE_UNKNOWN| - Unknown error (e.g., if not enough information
//       is available for a more specific error).
//   |MOJO_ERROR_CODE_INVALID_ARGUMENT| - Caller specified an invalid argument.
//       This differs from |MOJO_ERROR_CODE_FAILED_PRECONDITION| in that the
//       former indicates arguments that are invalid regardless of the state of
//       the system.
//   |MOJO_ERROR_CODE_DEADLINE_EXCEEDED| - Deadline expired before the operation
//       could complete.
//   |MOJO_ERROR_CODE_NOT_FOUND| - Some requested entity was not found (i.e.,
//       does not exist).
//   |MOJO_ERROR_CODE_ALREADY_EXISTS| - Some entity or condition that we
//       attempted to create already exists.
//   |MOJO_ERROR_CODE_PERMISSION_DENIED| - The caller does not have permission
//       to for the operation (use |MOJO_ERROR_CODE_RESOURCE_EXHAUSTED| for
//       rejections caused by exhausting some resource instead).
//   |MOJO_ERROR_CODE_RESOURCE_EXHAUSTED| - Some resource required for the call
//       (possibly some quota) has been exhausted.
//   |MOJO_ERROR_CODE_FAILED_PRECONDITION| - The system is not in a state
//       required for the operation (use this if the caller must do something to
//       rectify the state before retrying).
//   |MOJO_ERROR_CODE_ABORTED| - The operation was aborted by the system,
//       possibly due to a concurrency issue (use this if the caller may retry
//       at a higher level).
//   |MOJO_ERROR_CODE_OUT_OF_RANGE| - The operation was attempted past the valid
//       range. Unlike |MOJO_ERROR_CODE_INVALID_ARGUMENT|, this indicates that
//       the operation may be/become valid depending on the system state. (This
//       error is similar to |MOJO_ERROR_CODE_FAILED_PRECONDITION|, but is more
//       specific.)
//   |MOJO_ERROR_CODE_UNIMPLEMENTED| - The operation is not implemented,
//       supported, or enabled.
//   |MOJO_ERROR_CODE_INTERNAL| - Internal error: this should never happen and
//       indicates that some invariant expected by the system has been broken.
//   |MOJO_ERROR_CODE_UNAVAILABLE| - The operation is (temporarily) currently
//       unavailable. The caller may simply retry the operation (possibly with a
//       backoff).
//   |MOJO_ERROR_CODE_DATA_LOSS| - Unrecoverable data loss or corruption.
//
// The codes correspond to Google3's canonical error codes.

#define MOJO_ERROR_CODE_OK ((MojoResult)0x0u)
#define MOJO_ERROR_CODE_CANCELLED ((MojoResult)0x1u)
#define MOJO_ERROR_CODE_UNKNOWN ((MojoResult)0x2u)
#define MOJO_ERROR_CODE_INVALID_ARGUMENT ((MojoResult)0x3u)
#define MOJO_ERROR_CODE_DEADLINE_EXCEEDED ((MojoResult)0x4u)
#define MOJO_ERROR_CODE_NOT_FOUND ((MojoResult)0x5u)
#define MOJO_ERROR_CODE_ALREADY_EXISTS ((MojoResult)0x6u)
#define MOJO_ERROR_CODE_PERMISSION_DENIED ((MojoResult)0x7u)
#define MOJO_ERROR_CODE_RESOURCE_EXHAUSTED ((MojoResult)0x8u)
#define MOJO_ERROR_CODE_FAILED_PRECONDITION ((MojoResult)0x9u)
#define MOJO_ERROR_CODE_ABORTED ((MojoResult)0xau)
#define MOJO_ERROR_CODE_OUT_OF_RANGE ((MojoResult)0xbu)
#define MOJO_ERROR_CODE_UNIMPLEMENTED ((MojoResult)0xcu)
#define MOJO_ERROR_CODE_INTERNAL ((MojoResult)0xdu)
#define MOJO_ERROR_CODE_UNAVAILABLE ((MojoResult)0xeu)
#define MOJO_ERROR_CODE_DATA_LOSS ((MojoResult)0xfu)

// System/generic error space --------------------------------------------------

#define MOJO_ERROR_SPACE_SYSTEM ((MojoResult)0x0000u)

// Common/generic subcode, valid for all error codes:
//   |MOJO_ERROR_SUBCODE_GENERIC| - No additional details about the error are
//       specified.
#define MOJO_ERROR_SUBCODE_GENERIC ((MojoResult)0x000u)

// Subcodes valid for |MOJO_ERROR_CODE_FAILED_PRECONDITION|:
//   |MOJO_ERROR_CODE_FAILED_PRECONDITION_SUBCODE_BUSY| - One of the resources
//       involved is currently being used (possibly on another thread) in a way
//       that prevents the current operation from proceeding, e.g., if the other
//       operation may result in the resource being invalidated. TODO(vtl): We
//       should probably get rid of this, and report "invalid argument" instead
//       (with a different subcode scheme). This is here now for ease of
//       conversion with the existing |MOJO_RESULT_BUSY|.
#define MOJO_ERROR_CODE_FAILED_PRECONDITION_SUBCODE_BUSY ((MojoResult)0x001u)

// Subcodes valid for MOJO_ERROR_CODE_UNAVAILABLE:
//   |MOJO_ERROR_CODE_UNAVAILABLE_SUBCODE_SHOULD_WAIT| - The request cannot
//       currently be completed (e.g., if the data requested is not yet
//       available). The caller should wait for it to be feasible using one of
//       the wait primitives.
#define MOJO_ERROR_CODE_UNAVAILABLE_SUBCODE_SHOULD_WAIT ((MojoResult)0x001u)

// Complete results:

// Generic results:
#define MOJO_RESULT_OK                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_OK, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_CANCELLED                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_CANCELLED, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_UNKNOWN                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_UNKNOWN, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_INVALID_ARGUMENT                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_INVALID_ARGUMENT, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_DEADLINE_EXCEEDED                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_DEADLINE_EXCEEDED, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_NOT_FOUND                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_NOT_FOUND, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_ALREADY_EXISTS                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_ALREADY_EXISTS, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_PERMISSION_DENIED                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_PERMISSION_DENIED, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_RESOURCE_EXHAUSTED                 \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_RESOURCE_EXHAUSTED, \
                   MOJO_ERROR_SPACE_SYSTEM, MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_FAILED_PRECONDITION                 \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_FAILED_PRECONDITION, \
                   MOJO_ERROR_SPACE_SYSTEM, MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_ABORTED                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_ABORTED, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_OUT_OF_RANGE                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_OUT_OF_RANGE, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_UNIMPLEMENTED                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_UNIMPLEMENTED, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_INTERNAL                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_INTERNAL, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_UNAVAILABLE                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_UNAVAILABLE, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)
#define MOJO_RESULT_DATA_LOSS                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_DATA_LOSS, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_SUBCODE_GENERIC)

// Specific results (for backwards compatibility):
#define MOJO_RESULT_BUSY                                \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_FAILED_PRECONDITION, \
                   MOJO_ERROR_SPACE_SYSTEM,             \
                   MOJO_ERROR_CODE_FAILED_PRECONDITION_SUBCODE_BUSY)
#define MOJO_RESULT_SHOULD_WAIT                                          \
  MOJO_RESULT_MAKE(MOJO_ERROR_CODE_UNAVAILABLE, MOJO_ERROR_SPACE_SYSTEM, \
                   MOJO_ERROR_CODE_UNAVAILABLE_SUBCODE_SHOULD_WAIT)

#endif  // MOJO_PUBLIC_C_SYSTEM_RESULT_H_
