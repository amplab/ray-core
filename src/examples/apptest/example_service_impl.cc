// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/apptest/example_service_impl.h"

namespace mojo {

ExampleServiceImpl::ExampleServiceImpl(InterfaceRequest<ExampleService> request)
    : binding_(this, request.Pass()) {
}

ExampleServiceImpl::~ExampleServiceImpl() {}

void ExampleServiceImpl::Ping(uint16_t ping_value,
                              const ExampleService::PingCallback& callback) {
  callback.Run(ping_value);
}

}  // namespace mojo
