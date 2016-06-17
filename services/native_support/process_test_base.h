// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_SUPPORT_PROCESS_TEST_BASE_H_
#define SERVICES_NATIVE_SUPPORT_PROCESS_TEST_BASE_H_

#include <type_traits>

#include "base/macros.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "mojo/services/native_support/interfaces/process.mojom-sync.h"

namespace native_support {

class ProcessTestBase : public mojo::test::ApplicationTestBase {
 public:
  ProcessTestBase();
  ~ProcessTestBase() override;

  void SetUp() override;

 protected:
  mojo::SynchronousInterfacePtr<Process>& process() { return process_; }

 private:
  mojo::SynchronousInterfacePtr<Process> process_;

  DISALLOW_COPY_AND_ASSIGN(ProcessTestBase);
};

}  // namespace native_support

#endif  // SERVICES_NATIVE_SUPPORT_PROCESS_TEST_BASE_H_
