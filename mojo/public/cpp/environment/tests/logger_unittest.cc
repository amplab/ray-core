// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/environment/logger.h"
#include "mojo/public/cpp/environment/environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

std::vector<std::string> g_log_msgs;
const MojoLogger kTestLogger = {
    [](MojoLogLevel log_level,
       const char* source_file,
       uint32_t source_line,
       const char* message) { g_log_msgs.push_back(message); },
    []() { return MOJO_LOG_LEVEL_INFO; }, [](MojoLogLevel lvl) {}};

TEST(LoggerTest, Basic) {
  const char kPath[] = "/fake/path/to/file.cc";

  const MojoLogger* const logger = Environment::GetDefaultLogger();

  logger->LogMessage(MOJO_LOG_LEVEL_VERBOSE - 1, kPath, 123,
                     "Logged at VERBOSE-1 level");
  logger->LogMessage(MOJO_LOG_LEVEL_VERBOSE, kPath, 123,
                     "Logged at VERBOSE level");
  logger->LogMessage(MOJO_LOG_LEVEL_INFO, kPath, 123, "Logged at INFO level");
  logger->LogMessage(MOJO_LOG_LEVEL_WARNING, kPath, 123,
                     "Logged at WARNING level");
  logger->LogMessage(MOJO_LOG_LEVEL_ERROR, kPath, 123, "Logged at ERROR level");

  // This should kill us:
  EXPECT_DEATH_IF_SUPPORTED({
    logger->LogMessage(MOJO_LOG_LEVEL_FATAL, kPath, 123,
                       "Logged at FATAL level");
  }, "");
}

TEST(LoggerTest, LogLevels) {
  const char kPath[] = "/fake/path/to/file.cc";

  const MojoLogger* const logger = Environment::GetDefaultLogger();

  for (MojoLogLevel log_level = MOJO_LOG_LEVEL_VERBOSE - 1;
       log_level <= MOJO_LOG_LEVEL_FATAL + 1;
       log_level++) {
    logger->SetMinimumLogLevel(log_level);

    if (log_level <= MOJO_LOG_LEVEL_FATAL)
      EXPECT_EQ(log_level, logger->GetMinimumLogLevel());
    else
      EXPECT_EQ(MOJO_LOG_LEVEL_FATAL, logger->GetMinimumLogLevel());

    logger->LogMessage(MOJO_LOG_LEVEL_VERBOSE - 1, kPath, 123,
                       "Logged at VERBOSE-1 level");
    logger->LogMessage(MOJO_LOG_LEVEL_VERBOSE, kPath, 123,
                       "Logged at VERBOSE level");
    logger->LogMessage(MOJO_LOG_LEVEL_INFO, kPath, 123, "Logged at INFO level");
    logger->LogMessage(MOJO_LOG_LEVEL_WARNING, kPath, 123,
                       "Logged at WARNING level");
    logger->LogMessage(MOJO_LOG_LEVEL_ERROR, kPath, 123,
                       "Logged at ERROR level");

    // This should kill us:
    EXPECT_DEATH_IF_SUPPORTED({
      logger->LogMessage(MOJO_LOG_LEVEL_FATAL, kPath, 123,
                         "Logged at FATAL level");
    }, "");
  }
}

TEST(LoggerTest, NoFile) {
  const MojoLogger* const logger = Environment::GetDefaultLogger();

  logger->LogMessage(MOJO_LOG_LEVEL_VERBOSE - 1, nullptr, 0,
                     "Logged at VERBOSE-1 level");
  logger->LogMessage(MOJO_LOG_LEVEL_VERBOSE, nullptr, 0,
                     "Logged at VERBOSE level");
  logger->LogMessage(MOJO_LOG_LEVEL_INFO, nullptr, 0, "Logged at INFO level");
  logger->LogMessage(MOJO_LOG_LEVEL_WARNING, nullptr, 0,
                     "Logged at WARNING level");
  logger->LogMessage(MOJO_LOG_LEVEL_ERROR, nullptr, 0, "Logged at ERROR level");

  // This should kill us:
  EXPECT_DEATH_IF_SUPPORTED({
    logger->LogMessage(MOJO_LOG_LEVEL_FATAL, nullptr, 0,
                       "Logged at FATAL level");
  }, "");
}

TEST(LoggerTest, SetDefaultLogger) {
  Environment::SetDefaultLogger(&kTestLogger);

  std::vector<std::string> expected_msgs;
  expected_msgs.push_back("First message!");
  expected_msgs.push_back("Second message!");
  expected_msgs.push_back("Third message!");
  Environment::GetDefaultLogger()->LogMessage(MOJO_LOG_LEVEL_INFO, nullptr, 0,
                                              expected_msgs[0].c_str());
  Environment::GetDefaultLogger()->LogMessage(MOJO_LOG_LEVEL_INFO, nullptr, 0,
                                              expected_msgs[1].c_str());
  Environment::GetDefaultLogger()->LogMessage(MOJO_LOG_LEVEL_INFO, nullptr, 0,
                                              expected_msgs[2].c_str());

  EXPECT_EQ(expected_msgs, g_log_msgs);

  // This should restore the default logger.
  Environment::SetDefaultLogger(nullptr);

  EXPECT_NE(&kTestLogger, Environment::GetDefaultLogger());
  EXPECT_NE(nullptr, Environment::GetDefaultLogger());
}

}  // namespace
}  // namespace mojo
