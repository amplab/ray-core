// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// These are helper classes for allocating, aligning, and framing a
// |mojo::Message|. They are mainly used from within the generated C++ bindings.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_MESSAGE_BUILDER_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_MESSAGE_BUILDER_H_

#include <stdint.h>

#include "mojo/public/cpp/bindings/lib/fixed_buffer.h"
#include "mojo/public/cpp/bindings/lib/message_internal.h"
#include "mojo/public/cpp/bindings/message.h"

namespace mojo {

class Message;

// MessageBuilder helps initialize and frame a |mojo::Message| that does not
// expect a response message, and therefore does not tag the message with a
// request id (which may save some bytes).
//
// The underlying |Message| is owned by MessageBuilder, but can be permanently
// moved by accessing |message()| and calling its |MoveTo()|.
class MessageBuilder {
 public:
  // This frames and configures a |mojo::Message| with the given message name.
  MessageBuilder(uint32_t name, size_t payload_size);
  ~MessageBuilder();

  Message* message() { return &message_; }

  // TODO(vardhan): |buffer()| is internal and only consumed by internal classes
  // and unittests.  Consider making it private + friend class its consumers?
  internal::Buffer* buffer() { return &buf_; }

 protected:
  MessageBuilder();
  void Initialize(size_t size);

  Message message_;
  internal::FixedBuffer buf_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(MessageBuilder);
};

namespace internal {

class MessageWithRequestIDBuilder : public MessageBuilder {
 public:
  MessageWithRequestIDBuilder(uint32_t name,
                              size_t payload_size,
                              uint32_t flags,
                              uint64_t request_id);
};

}  // namespace internal

// Builds a |mojo::Message| that is a "request" message that expects a response
// message. You can give it a unique |request_id| (with which you can construct
// a response message) by calling |message()->set_request_id()|.
//
// Has the same interface as |mojo::MessageBuilder|.
class RequestMessageBuilder : public internal::MessageWithRequestIDBuilder {
 public:
  RequestMessageBuilder(uint32_t name, size_t payload_size)
      : MessageWithRequestIDBuilder(name,
                                    payload_size,
                                    internal::kMessageExpectsResponse,
                                    0) {}
};

// Builds a |mojo::Message| that is a "response" message which pertains to a
// |request_id|.
//
// Has the same interface as |mojo::MessageBuilder|.
class ResponseMessageBuilder : public internal::MessageWithRequestIDBuilder {
 public:
  ResponseMessageBuilder(uint32_t name,
                         size_t payload_size,
                         uint64_t request_id)
      : MessageWithRequestIDBuilder(name,
                                    payload_size,
                                    internal::kMessageIsResponse,
                                    request_id) {}
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_MESSAGE_BUILDER_H_
