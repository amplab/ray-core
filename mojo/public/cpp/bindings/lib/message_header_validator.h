// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_MESSAGE_HEADER_VALIDATOR_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_MESSAGE_HEADER_VALIDATOR_H_

#include <string>

#include "mojo/public/cpp/bindings/lib/validation_errors.h"
#include "mojo/public/cpp/bindings/message_validator.h"

namespace mojo {
namespace internal {

class MessageHeaderValidator final : public MessageValidator {
 public:
  ValidationError Validate(const Message* message, std::string* err) override;
};

// The following methods validate control messages defined in
// interface_control_messages.mojom.
ValidationError ValidateControlRequest(const Message* message,
                                       std::string* err);
ValidationError ValidateControlResponse(const Message* message,
                                        std::string* err);

}  // namespace internal
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_MESSAGE_HEADER_VALIDATOR_H_
