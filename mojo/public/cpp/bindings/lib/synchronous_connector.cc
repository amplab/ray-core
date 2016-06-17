// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/synchronous_connector.h"

#include <utility>

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/time.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/environment/logging.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/wait.h"

namespace mojo {
namespace internal {

SynchronousConnector::SynchronousConnector(ScopedMessagePipeHandle handle)
    : handle_(std::move(handle)) {}

SynchronousConnector::~SynchronousConnector() {}

bool SynchronousConnector::Write(Message* msg_to_send) {
  MOJO_DCHECK(handle_.is_valid());
  MOJO_DCHECK(msg_to_send);

  auto result = WriteMessageRaw(
      handle_.get(), msg_to_send->data(), msg_to_send->data_num_bytes(),
      msg_to_send->mutable_handles()->empty()
          ? nullptr
          : reinterpret_cast<const MojoHandle*>(
                msg_to_send->mutable_handles()->data()),
      static_cast<uint32_t>(msg_to_send->mutable_handles()->size()),
      MOJO_WRITE_MESSAGE_FLAG_NONE);

  switch (result) {
    case MOJO_RESULT_OK:
      break;

    case MOJO_RESULT_INVALID_ARGUMENT:
    case MOJO_RESULT_RESOURCE_EXHAUSTED:
    case MOJO_RESULT_FAILED_PRECONDITION:
    case MOJO_RESULT_UNIMPLEMENTED:
    case MOJO_RESULT_BUSY:
    default:
      MOJO_LOG(WARNING) << "WriteMessageRaw unsuccessful. error = " << result;
      return false;
  }

  return true;
}

bool SynchronousConnector::BlockingRead(Message* received_msg) {
  MOJO_DCHECK(handle_.is_valid());
  MOJO_DCHECK(received_msg);

  MojoResult rv = Wait(handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE, nullptr);

  if (rv != MOJO_RESULT_OK) {
    MOJO_LOG(WARNING) << "Failed waiting for a response. error = " << rv;
    return false;
  }

  rv = ReadMessage(handle_.get(), received_msg);
  if (rv != MOJO_RESULT_OK) {
    MOJO_LOG(WARNING) << "Failed reading the response message. error = " << rv;
    return false;
  }

  return true;
}

}  // namespace internal
}  // namespace mojo
