// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/indirect_service/indirect_service_demo.mojom.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace mojo {
namespace examples {

class IntegerServiceImpl : public IntegerService {
 public:
  explicit IntegerServiceImpl(InterfaceRequest<IntegerService> request)
      : value_(0), binding_(this, request.Pass()) {}
  ~IntegerServiceImpl() override {}

  void Increment(const Callback<void(int32_t)>& callback) override {
    callback.Run(value_++);
  }

 private:
  int32_t value_;
  StrongBinding<IntegerService> binding_;
};

class IntegerServiceApp : public ApplicationImplBase {
 public:
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<IntegerService>(
        [](const ConnectionContext& connection_context,
           InterfaceRequest<IntegerService> request) {
          new IntegerServiceImpl(request.Pass());
        });
    return true;
  }
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::examples::IntegerServiceApp integer_service_app;
  return mojo::RunApplication(application_request, &integer_service_app);
}

