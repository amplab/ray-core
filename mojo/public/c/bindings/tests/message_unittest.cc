// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/bindings/message.h"

#include <stdint.h>

#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/tests/validation_test_input_parser.h"
#include "mojo/public/cpp/system/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(MessageValidationTest, InvalidMessageHeader) {
  struct TestCase {
    const char* test_case;
    uint32_t size;
    const char* name;
    MojomValidationResult validation_result;
  };

  TestCase cases[] = {
      {"[u4]8  // num_bytes\n"
       "[u4]0  // version",
       16u, "num_bytes too small", MOJOM_VALIDATION_UNEXPECTED_STRUCT_HEADER},
      {"[u4]24 // num_bytes\n"
       "[u4]0  // version\n"
       "[u4]0  // name\n"
       "[u4]0  // flags\n"
       "[u8]0  // request id",
       24u, "version 0 header with version 1 size",
       MOJOM_VALIDATION_UNEXPECTED_STRUCT_HEADER},
      {"[u4]16 // num_bytes\n"
       "[u4]1  // version\n"
       "[u4]0  // name\n"
       "[u4]0  // flags",
       20u, "version 1 header with version 0 size",
       MOJOM_VALIDATION_UNEXPECTED_STRUCT_HEADER},
      {"[u4]16 // num_bytes\n"
       "[u4]0  // version\n"
       "[u4]0  // name\n"
       "[u4]1  // flags",
       16u, "version 0 header with expect response flag",
       MOJOM_VALIDATION_MESSAGE_HEADER_MISSING_REQUEST_ID},
      {"[u4]16 // num_bytes\n"
       "[u4]0  // version\n"
       "[u4]0  // name\n"
       "[u4]2  // flags",
       16u, "version 0 header with is response flag",
       MOJOM_VALIDATION_MESSAGE_HEADER_MISSING_REQUEST_ID},
      {"[u4]16 // num_bytes\n"
       "[u4]0  // version\n"
       "[u4]0  // name\n"
       "[u4]3  // flags",
       16u, "version 0 header with both is/expects response flags",
       MOJOM_VALIDATION_MESSAGE_HEADER_MISSING_REQUEST_ID},
      {"[u4]24 // num_bytes\n"
       "[u4]1  // version\n"
       "[u4]0  // name\n"
       "[u4]3  // flags\n"
       "[u8]0  // request id",
       24u, "version 1 header with both is/expects response flags",
       MOJOM_VALIDATION_MESSAGE_HEADER_INVALID_FLAGS},
  };
  for (size_t i = 0u; i < MOJO_ARRAYSIZE(cases); ++i) {
    std::vector<uint8_t> data;
    std::string parser_error_message;
    size_t num_handles = 0u;
    ASSERT_TRUE(mojo::test::ParseValidationTestInput(
        cases[i].test_case, &data, &num_handles, &parser_error_message))
        << parser_error_message << " case " << i;
    EXPECT_EQ(cases[i].validation_result,
              MojomMessage_ValidateHeader(data.data(), cases[i].size))
        << cases[i].name << " case " << i;
  }
}

TEST(MessageValidationTest, ValidMessageHeader) {
  struct TestCase {
    const char* test_case;
    size_t size;
  };

  TestCase cases[] = {
      {"[u4]16 // num_bytes\n"
       "[u4]0  // version\n"
       "[u4]0  // name\n"
       "[u4]0  // flags",
       16u},  // version 0 header
      {"[u4]24 // num_bytes\n"
       "[u4]1  // version\n"
       "[u4]0  // name\n"
       "[u4]1  // flags\n"
       "[u8]0  // request_id",
       24u},  // version 1 request
      {"[u4]24 // num_bytes\n"
       "[u4]1  // version\n"
       "[u4]0  // name\n"
       "[u4]2  // flags\n"
       "[u8]0  // request_id",
       24u},  // version 1 response
      {"[u4]24 // num_bytes\n"
       "[u4]1  // version\n"
       "[u4]0  // name\n"
       "[u4]0  // flags\n"
       "[u8]0  // request id",
       24u},  // version 1 header without is/expects response flags
  };

  for (size_t i = 0u; i < MOJO_ARRAYSIZE(cases); ++i) {
    std::vector<uint8_t> data;
    std::string parser_error_message;
    size_t num_handles = 0u;
    ASSERT_TRUE(mojo::test::ParseValidationTestInput(
        cases[i].test_case, &data, &num_handles, &parser_error_message))
        << parser_error_message << " case " << i;
    EXPECT_EQ(MOJOM_VALIDATION_ERROR_NONE,
              MojomMessage_ValidateHeader(data.data(), cases[i].size))
        << " case " << i;
  }
}

}  // namespace
