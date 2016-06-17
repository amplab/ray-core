// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_MESSAGE_VALIDATOR_H_
#define MOJO_PUBLIC_CPP_BINDINGS_MESSAGE_VALIDATOR_H_

#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/lib/validation_errors.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/system/macros.h"

namespace mojo {
namespace internal {

// This class is the base class for message validators. Subclasses should
// implement the pure virtual method Validate() to validate messages.
class MessageValidator {
 public:
  virtual ~MessageValidator() {}
  // Validates the message and returns ValidationError::NONE if valid.
  // Otherwise returns a different value, and sets the |err| string if it is not
  // null.
  virtual ValidationError Validate(const Message* message,
                                   std::string* err) = 0;
};

// A message validator that does nothing, and always returns a non-error.
class PassThroughValidator final : public MessageValidator {
 public:
  ValidationError Validate(const Message* message, std::string* err) override;
};

using MessageValidatorList = std::vector<std::unique_ptr<MessageValidator>>;

// Iterates through |validators| and tries to validate the given |message| until
// a validator fails.  If a validator fails, a |ValidationError| is returned and
// the supplied |err| is set if it is not null.  |ValidationError::NONE| is
// returned if the supplied |message| passes all the |validators|.
ValidationError RunValidatorsOnMessage(const MessageValidatorList& validators,
                                       const Message* message,
                                       std::string* err);

}  // namespace internal
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_MESSAGE_VALIDATOR_H_
