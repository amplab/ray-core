// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/test_service/test_request_tracker_application.h"

#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "services/test_service/test_time_service_impl.h"

namespace mojo {
namespace test {

TestRequestTrackerApplication::TestRequestTrackerApplication() {}

TestRequestTrackerApplication::~TestRequestTrackerApplication() {}

bool TestRequestTrackerApplication::OnAcceptConnection(
    ServiceProviderImpl* service_provider_impl) {
  // Every instance of the service and recorder shares the context.
  // Note, this app is single-threaded, so this is thread safe.
  service_provider_impl->AddService<TestTimeService>(
      [this](const ConnectionContext& connection_context,
             InterfaceRequest<TestTimeService> request) {
        new TestTimeServiceImpl(this, request.Pass());
      });
  service_provider_impl->AddService<TestRequestTracker>(
      [this](const ConnectionContext& connection_context,
             InterfaceRequest<TestRequestTracker> request) {
        new TestRequestTrackerImpl(request.Pass(), &context_);
      });
  service_provider_impl->AddService<TestTrackedRequestService>(
      [this](const ConnectionContext& connection_context,
             InterfaceRequest<TestTrackedRequestService> request) {
        new TestTrackedRequestServiceImpl(request.Pass(), &context_);
      });
  return true;
}

}  // namespace test
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::test::TestRequestTrackerApplication app;
  return mojo::RunApplication(application_request, &app);
}
