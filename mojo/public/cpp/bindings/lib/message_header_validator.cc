// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/message_header_validator.h"

#include <string>

#include "mojo/public/cpp/bindings/lib/bounds_checker.h"
#include "mojo/public/cpp/bindings/lib/message_validation.h"
#include "mojo/public/cpp/bindings/lib/validation_errors.h"
#include "mojo/public/cpp/bindings/lib/validation_util.h"
#include "mojo/public/interfaces/bindings/interface_control_messages.mojom.h"

namespace mojo {
namespace internal {
namespace {

ValidationError ValidateMessageHeader(const MessageHeader* header,
                                      std::string* err) {
  // NOTE: Our goal is to preserve support for future extension of the message
  // header. If we encounter fields we do not understand, we must ignore them.
  // Extra validation of the struct header:
  if (header->version == 0) {
    if (header->num_bytes != sizeof(MessageHeader)) {
      MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err)
          << "message header size is incorrect";
      return ValidationError::UNEXPECTED_STRUCT_HEADER;
    }
  } else if (header->version == 1) {
    if (header->num_bytes != sizeof(MessageHeaderWithRequestID)) {
      MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err)
          << "message header (version = 1) size is incorrect";
      return ValidationError::UNEXPECTED_STRUCT_HEADER;
    }
  } else if (header->version > 1) {
    if (header->num_bytes < sizeof(MessageHeaderWithRequestID)) {
      MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err)
          << "message header (version > 1) size is too small";
      return ValidationError::UNEXPECTED_STRUCT_HEADER;
    }
  }

  // Validate flags (allow unknown bits):

  // These flags require a RequestID.
  if (header->version < 1 && ((header->flags & kMessageExpectsResponse) ||
                              (header->flags & kMessageIsResponse))) {
    MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err)
        << "message header associates itself with a response but does not "
           "contain a request id";
    return ValidationError::MESSAGE_HEADER_MISSING_REQUEST_ID;
  }

  // These flags are mutually exclusive.
  if ((header->flags & kMessageExpectsResponse) &&
      (header->flags & kMessageIsResponse)) {
    MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err)
        << "message header cannot indicate itself as a response while also "
           "expecting a response";
    return ValidationError::MESSAGE_HEADER_INVALID_FLAGS;
  }

  return ValidationError::NONE;
}

}  // namespace

ValidationError MessageHeaderValidator::Validate(const Message* message,
                                                 std::string* err) {
  // Pass 0 as number of handles because we don't expect any in the header, even
  // if |message| contains handles.
  BoundsChecker bounds_checker(message->data(), message->data_num_bytes(), 0);

  ValidationError result =
      ValidateStructHeaderAndClaimMemory(message->data(), &bounds_checker, err);
  if (result != ValidationError::NONE)
    return result;

  return ValidateMessageHeader(message->header(), err);
}

ValidationError ValidateControlRequest(const Message* message,
                                       std::string* err) {
  ValidationError retval;
  switch (message->header()->name) {
    case kRunMessageId: {
      retval = ValidateMessageIsRequestExpectingResponse(message, err);
      if (retval != ValidationError::NONE)
        return retval;

      return ValidateMessagePayload<RunMessageParams_Data>(message, err);
    }

    case kRunOrClosePipeMessageId: {
      retval = ValidateMessageIsRequestWithoutResponse(message, err);
      if (retval != ValidationError::NONE)
        return retval;

      return ValidateMessagePayload<RunOrClosePipeMessageParams_Data>(message,
                                                                      err);
    }

    default: {
      MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err)
          << "unknown InterfaceControlMessage request message name: "
          << message->header()->name;
      return ValidationError::MESSAGE_HEADER_UNKNOWN_METHOD;
    }
  }

  return ValidationError::NONE;
}

ValidationError ValidateControlResponse(const Message* message,
                                        std::string* err) {
  ValidationError retval = ValidateMessageIsResponse(message, err);
  if (retval != ValidationError::NONE)
    return retval;

  switch (message->header()->name) {
    case kRunMessageId:
      return ValidateMessagePayload<RunResponseMessageParams_Data>(message,
                                                                   err);
  }

  MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err)
      << "unknown InterfaceControlMessage response message name: "
      << message->header()->name;
  return ValidationError::MESSAGE_HEADER_UNKNOWN_METHOD;
}

}  // namespace internal
}  // namespace mojo
