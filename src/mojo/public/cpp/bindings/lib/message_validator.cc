// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/message_validator.h"

namespace mojo {
namespace internal {

ValidationError PassThroughValidator::Validate(const Message* message,
                                               std::string* err) {
  return ValidationError::NONE;
}

ValidationError RunValidatorsOnMessage(const MessageValidatorList& validators,
                                       const Message* message,
                                       std::string* err) {
  for (const auto& validator : validators) {
    auto result = validator->Validate(message, err);
    if (result != ValidationError::NONE)
      return result;
  }

  return ValidationError::NONE;
}

}  // namespace internal
}  // namespace mojo
