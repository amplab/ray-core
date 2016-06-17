// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/message.h"

#include <stdlib.h>

#include <algorithm>

#include "mojo/public/cpp/environment/logging.h"

namespace mojo {

Message::Message() {
  Initialize();
}

Message::~Message() {
  FreeDataAndCloseHandles();
}

void Message::Reset() {
  FreeDataAndCloseHandles();

  handles_.clear();
  Initialize();
}

void Message::AllocData(uint32_t num_bytes) {
  MOJO_DCHECK(!data_);
  data_num_bytes_ = num_bytes;
  data_ = static_cast<internal::MessageData*>(calloc(num_bytes, 1));
}

void Message::AllocUninitializedData(uint32_t num_bytes) {
  MOJO_DCHECK(!data_);
  data_num_bytes_ = num_bytes;
  data_ = static_cast<internal::MessageData*>(malloc(num_bytes));
}

void Message::MoveTo(Message* destination) {
  MOJO_DCHECK(this != destination);

  destination->FreeDataAndCloseHandles();

  // No copy needed.
  destination->data_num_bytes_ = data_num_bytes_;
  destination->data_ = data_;
  std::swap(destination->handles_, handles_);

  handles_.clear();
  Initialize();
}

void Message::Initialize() {
  data_num_bytes_ = 0;
  data_ = nullptr;
}

void Message::FreeDataAndCloseHandles() {
  free(data_);

  for (std::vector<Handle>::iterator it = handles_.begin();
       it != handles_.end(); ++it) {
    if (it->is_valid())
      CloseRaw(*it);
  }
}

MojoResult ReadMessage(MessagePipeHandle handle, Message* message) {
  MOJO_DCHECK(handle.is_valid());
  MOJO_DCHECK(message);
  MOJO_DCHECK(message->handles()->empty());
  MOJO_DCHECK(message->data_num_bytes() == 0);

  uint32_t num_bytes = 0;
  uint32_t num_handles = 0;
  MojoResult rv = ReadMessageRaw(handle, nullptr, &num_bytes, nullptr,
                                 &num_handles, MOJO_READ_MESSAGE_FLAG_NONE);
  if (rv != MOJO_RESULT_RESOURCE_EXHAUSTED)
    return rv;

  message->AllocUninitializedData(num_bytes);
  message->mutable_handles()->resize(num_handles);

  uint32_t num_bytes_actual = num_bytes;
  uint32_t num_handles_actual = num_handles;
  rv = ReadMessageRaw(
      handle, message->mutable_data(), &num_bytes_actual,
      message->mutable_handles()->empty()
          ? nullptr
          : reinterpret_cast<MojoHandle*>(&message->mutable_handles()->front()),
      &num_handles_actual, MOJO_READ_MESSAGE_FLAG_NONE);

  MOJO_DCHECK(num_bytes == num_bytes_actual);
  MOJO_DCHECK(num_handles == num_handles_actual);

  return rv;
}

MojoResult ReadAndDispatchMessage(MessagePipeHandle handle,
                                  MessageReceiver* receiver,
                                  bool* receiver_result) {
  Message message;
  MojoResult rv = ReadMessage(handle, &message);
  if (receiver && rv == MOJO_RESULT_OK)
    *receiver_result = receiver->Accept(&message);

  return rv;
}

}  // namespace mojo
