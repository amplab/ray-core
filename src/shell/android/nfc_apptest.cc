// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/callback.h"
#include "mojo/public/cpp/environment/logging.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/nfc/interfaces/nfc.mojom.h"

namespace mojo {
namespace {

// Tests the NFC mojo service on android.
class NfcApplicationTest : public test::ApplicationTestBase {
 public:
  NfcApplicationTest() : ApplicationTestBase() {}
  ~NfcApplicationTest() override {}

  void TransmitOnNextConnectionResult(bool success) {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();
    ConnectToService(shell(), "mojo:nfc", GetProxy(&nfc_));
  }

  nfc::NfcPtr nfc_;
  nfc::NfcTransmissionPtr nfc_transmission_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(NfcApplicationTest);
};

TEST_F(NfcApplicationTest, TransmitAndCancelNfc) {
  nfc::NfcDataPtr nfc_data = nfc::NfcData::New();
  nfc_->TransmitOnNextConnection(
      nfc_data.Pass(), GetProxy(&nfc_transmission_),
      base::Bind(&NfcApplicationTest::TransmitOnNextConnectionResult,
                 base::Unretained(this)));
  nfc_transmission_->Cancel();
}

TEST_F(NfcApplicationTest, Register) {
  nfc_->Register();
}

}  // namespace
}  // namespace mojo
