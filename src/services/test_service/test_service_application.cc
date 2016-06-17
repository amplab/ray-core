// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/test_service/test_service_application.h"

#include <assert.h>

#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "services/test_service/test_service_impl.h"
#include "services/test_service/test_time_service_impl.h"

namespace mojo {
namespace test {

TestServiceApplication::TestServiceApplication() : ref_count_(0) {}

TestServiceApplication::~TestServiceApplication() {}

bool TestServiceApplication::OnAcceptConnection(
    ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<TestService>(
      [this](const ConnectionContext& connection_context,
             InterfaceRequest<TestService> request) {
        new TestServiceImpl(this, request.Pass());
        AddRef();
      });
  service_provider_impl->AddService<TestTimeService>(
      [this](const ConnectionContext& connection_context,
             InterfaceRequest<TestTimeService> request) {
        new TestTimeServiceImpl(this, request.Pass());
      });
  return true;
}

void TestServiceApplication::AddRef() {
  assert(ref_count_ >= 0);
  ref_count_++;
}

void TestServiceApplication::ReleaseRef() {
  assert(ref_count_ > 0);
  ref_count_--;
  if (ref_count_ <= 0)
    TerminateApplication(MOJO_RESULT_OK);
}

}  // namespace test
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::test::TestServiceApplication app;
  return mojo::RunApplication(application_request, &app);
}
