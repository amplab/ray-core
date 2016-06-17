// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TEST_SERVICE_TEST_SERVICE_APPLICATION_H_
#define SERVICES_TEST_SERVICE_TEST_SERVICE_APPLICATION_H_

#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/system/macros.h"

namespace mojo {
namespace test {

class TestServiceApplication : public ApplicationImplBase {
 public:
  TestServiceApplication();
  ~TestServiceApplication() override;

  // |ApplicationImplBase| method:
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override;

  void AddRef();
  void ReleaseRef();

 private:
  int ref_count_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(TestServiceApplication);
};

}  // namespace test
}  // namespace mojo

#endif  // SERVICES_TEST_SERVICE_TEST_SERVICE_APPLICATION_H_
