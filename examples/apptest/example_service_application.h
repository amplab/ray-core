// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EXAMPLES_TEST_EXAMPLE_SERVICE_APPLICATION_H_
#define MOJO_EXAMPLES_TEST_EXAMPLE_SERVICE_APPLICATION_H_

#include "examples/apptest/example_service_impl.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/system/macros.h"

namespace mojo {

class ExampleServiceApplication : public ApplicationImplBase {
 public:
  ExampleServiceApplication();
  ~ExampleServiceApplication() override;

 private:
  // ApplicationImplBase override.
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ExampleServiceApplication);
};

}  // namespace mojo

#endif  // MOJO_EXAMPLES_TEST_EXAMPLE_SERVICE_APPLICATION_H_
