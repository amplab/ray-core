// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_SYNCHRONOUS_CONNECTOR_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_SYNCHRONOUS_CONNECTOR_H_

#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {
namespace internal {

// This class is responsible for performing synchronous read/write operations on
// a MessagePipe. Notably, this interface allows us to make synchronous
// request/response operations on messages: write a message (that expects a
// response message), wait on the message pipe, and read the response.
class SynchronousConnector {
 public:
  explicit SynchronousConnector(ScopedMessagePipeHandle handle);
  ~SynchronousConnector();

  // This will mutate the message by moving the handles out of it. |msg_to_send|
  // must be non-null. Returns true on a successful write.
  bool Write(Message* msg_to_send);

  // This method blocks indefinitely until a message is received. |received_msg|
  // must be non-null and be empty. Returns true on a successful read.
  // TODO(vardhan): Add a timeout mechanism.
  bool BlockingRead(Message* received_msg);

  ScopedMessagePipeHandle PassHandle() { return std::move(handle_); }

  // Returns true if the underlying MessagePipe is valid.
  bool is_valid() const { return handle_.is_valid(); }

 private:
  ScopedMessagePipeHandle handle_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(SynchronousConnector);
};

}  // namespace internal
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_SYNCHRONOUS_CONNECTOR_H_
