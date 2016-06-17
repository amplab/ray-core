// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TEST_SERVICE_TEST_REQUEST_TRACKER_APPLICATION_H_
#define SERVICES_TEST_SERVICE_TEST_REQUEST_TRACKER_APPLICATION_H_

#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/system/macros.h"
#include "services/test_service/test_request_tracker_impl.h"

namespace mojo {
namespace test {

class TestTimeService;

// Embeds TestRequestTracker mojo services into an application.
class TestRequestTrackerApplication : public ApplicationImplBase {
 public:
  TestRequestTrackerApplication();
  ~TestRequestTrackerApplication() override;

  // |ApplicationImplBase| method:
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override;

 private:
  TrackingContext context_;
  MOJO_DISALLOW_COPY_AND_ASSIGN(TestRequestTrackerApplication);
};

}  // namespace test
}  // namespace mojo

#endif  // SERVICES_TEST_SERVICE_TEST_REQUEST_TRACKER_APPLICATION_H_
