// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/test/test_timeouts.h"
#include "mojo/public/c/environment/logger.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/connection_context.h"
#include "mojo/public/cpp/system/time.h"
#include "mojo/services/log/interfaces/entry.mojom.h"
#include "mojo/services/log/interfaces/log.mojom.h"
#include "services/log/log_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace log {
namespace {

using base::MessageLoop;
using LogImplTest = mojo::test::ApplicationTestBase;

// Tests the Log service implementation by calling its AddEntry and verifying
// the log message that it "prints".
TEST_F(LogImplTest, AddEntryOutput) {
  std::vector<std::string> messages;

  LogPtr log;
  ConnectionContext connection_context(ConnectionContext::Type::INCOMING,
                                       "mojo:log_impl_unittest", "mojo:log");
  LogImpl::Create(
      connection_context, GetProxy(&log),
      [&messages](const std::string& message) { messages.push_back(message); });

  Entry entry;
  entry.log_level = MOJO_LOG_LEVEL_INFO;
  entry.timestamp = GetTimeTicksNow();
  entry.source_file = "file.ext";
  entry.source_line = 0;
  entry.message = "1234567890";
  log->AddEntry(entry.Clone());

  entry.source_line = 1;
  log->AddEntry(entry.Clone());

  entry.source_file.reset();
  log->AddEntry(entry.Clone());

  entry.message.reset();
  log->AddEntry(entry.Clone());

  log.reset();

  MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                          MessageLoop::QuitWhenIdleClosure(),
                                          TestTimeouts::tiny_timeout());

  MessageLoop::current()->Run();

  const std::vector<std::string> kExpectedMessages = {
      "<mojo:log_impl_unittest> [INFO] file.ext: 1234567890",
      "<mojo:log_impl_unittest> [INFO] file.ext:1: 1234567890",
      "<mojo:log_impl_unittest> [INFO] 1234567890",
      "<mojo:log_impl_unittest> [INFO] <no message>",
  };
  EXPECT_EQ(kExpectedMessages, messages);
}

}  // namespace
}  // namespace log
}  // namespace mojo
