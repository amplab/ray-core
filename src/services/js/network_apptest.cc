// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "mojo/public/cpp/application/connect.h"
#include "services/js/test/js_application_test_base.h"
#include "services/js/test/network_test_service.mojom.h"

namespace js {
namespace {

class JSNetworkTest : public test::JSApplicationTestBase {
 public:
  JSNetworkTest() : JSApplicationTestBase() {}
  ~JSNetworkTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();
    const std::string& url = JSAppURL("network_test.js");
    mojo::ConnectToService(shell(), url, GetProxy(&network_test_service_));
  }

  NetworkTestServicePtr network_test_service_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(JSNetworkTest);
};

struct GetFileSizeCallback {
  GetFileSizeCallback(bool* b, uint64* s) : ok(b), size(s) {}
  void Run(bool ok_value, uint64 size_value) const {
    *ok = ok_value;
    *size = size_value;
  }
  bool* ok;
  uint64* size;
};

// Verify that JS can connect to the network service's URLLoader and use
// Core.drainData() to load a file's contents. This test reproduces what
// examples/wget.js does.
TEST_F(JSNetworkTest, GetFileSize) {
  const std::string filename("network_test.js");
  int64 expected_file_size;
  EXPECT_TRUE(GetFileSize(JSAppPath(filename), &expected_file_size));
  EXPECT_TRUE(expected_file_size > 0);

  bool file_size_ok;
  uint64 file_size;
  GetFileSizeCallback callback(&file_size_ok, &file_size);
  network_test_service_->GetFileSize(JSAppURL(filename), callback);
  EXPECT_TRUE(network_test_service_.WaitForIncomingResponse());
  EXPECT_TRUE(file_size_ok);
  EXPECT_EQ(file_size, static_cast<uint64>(expected_file_size));
  network_test_service_->Quit();
}

}  // namespace
}  // namespace js

