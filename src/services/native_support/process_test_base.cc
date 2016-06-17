// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_support/process_test_base.h"

#include "mojo/public/cpp/application/connect.h"
#include "mojo/services/native_support/interfaces/process.mojom.h"

using mojo::SynchronousInterfacePtr;

namespace native_support {

ProcessTestBase::ProcessTestBase() {}

ProcessTestBase::~ProcessTestBase() {}

void ProcessTestBase::SetUp() {
  mojo::test::ApplicationTestBase::SetUp();
  mojo::ConnectToService(shell(), "mojo:native_support",
                         GetSynchronousProxy(&process_));
}

}  // namespace native_support
