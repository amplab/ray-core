// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/test_support/test_utils.h"

#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/wait.h"

namespace mojo {
namespace test {

bool WriteTextMessage(const MessagePipeHandle& handle,
                      const std::string& text) {
  MojoResult rv =
      WriteMessageRaw(handle, text.data(), static_cast<uint32_t>(text.size()),
                      nullptr, 0u, MOJO_WRITE_MESSAGE_FLAG_NONE);
  return rv == MOJO_RESULT_OK;
}

bool ReadTextMessage(const MessagePipeHandle& handle, std::string* text) {
  MojoResult rv;
  bool did_wait = false;

  uint32_t num_bytes = 0u;
  uint32_t num_handles = 0u;
  for (;;) {
    rv = ReadMessageRaw(handle, nullptr, &num_bytes, nullptr, &num_handles,
                        MOJO_READ_MESSAGE_FLAG_NONE);
    if (rv == MOJO_RESULT_SHOULD_WAIT) {
      if (did_wait) {
        assert(false);  // Looping endlessly!?
        return false;
      }
      rv = Wait(handle, MOJO_HANDLE_SIGNAL_READABLE, MOJO_DEADLINE_INDEFINITE,
                nullptr);
      if (rv != MOJO_RESULT_OK)
        return false;
      did_wait = true;
    } else {
      assert(!num_handles);
      break;
    }
  }

  text->resize(num_bytes);
  rv = ReadMessageRaw(handle, &text->at(0), &num_bytes, nullptr, &num_handles,
                      MOJO_READ_MESSAGE_FLAG_NONE);
  return rv == MOJO_RESULT_OK;
}

bool DiscardMessage(const MessagePipeHandle& handle) {
  MojoResult rv = ReadMessageRaw(handle, nullptr, nullptr, nullptr, nullptr,
                                 MOJO_READ_MESSAGE_FLAG_MAY_DISCARD);
  return rv == MOJO_RESULT_OK;
}

}  // namespace test
}  // namespace mojo
