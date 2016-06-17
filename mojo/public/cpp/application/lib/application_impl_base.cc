// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/application_impl_base.h"

#include <algorithm>
#include <utility>

#include "mojo/public/cpp/application/connection_context.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/environment/logging.h"

namespace mojo {

ApplicationImplBase::~ApplicationImplBase() {}

void ApplicationImplBase::Bind(
    InterfaceRequest<Application> application_request) {
  application_binding_.Bind(application_request.Pass());
}

bool ApplicationImplBase::HasArg(const std::string& arg) const {
  return std::find(args_.begin(), args_.end(), arg) != args_.end();
}

void ApplicationImplBase::OnInitialize() {}

bool ApplicationImplBase::OnAcceptConnection(
    ServiceProviderImpl* service_provider_impl) {
  return false;
}

void ApplicationImplBase::OnQuit() {}

void ApplicationImplBase::Terminate(MojoResult result) {
  TerminateApplication(result);
}

ApplicationImplBase::ApplicationImplBase() : application_binding_(this) {}

void ApplicationImplBase::Initialize(InterfaceHandle<Shell> shell,
                                     Array<String> args,
                                     const mojo::String& url) {
  shell_ = ShellPtr::Create(std::move(shell));
  shell_.set_connection_error_handler([this]() {
    OnQuit();
    service_provider_impls_.clear();
    // TODO(vtl): Maybe this should be |MOJO_RESULT_UNKNOWN| or something else,
    // but currently tests fail if we don't just report "OK".
    Terminate(MOJO_RESULT_OK);
  });
  url_ = url;
  args_ = args.To<std::vector<std::string>>();
  OnInitialize();
}

void ApplicationImplBase::AcceptConnection(
    const String& requestor_url,
    const String& url,
    InterfaceRequest<ServiceProvider> services) {
  std::unique_ptr<ServiceProviderImpl> service_provider_impl(
      new ServiceProviderImpl(
          ConnectionContext(ConnectionContext::Type::INCOMING, requestor_url,
                            url),
          services.Pass()));
  if (!OnAcceptConnection(service_provider_impl.get()))
    return;
  service_provider_impls_.push_back(std::move(service_provider_impl));
}

void ApplicationImplBase::RequestQuit() {
  OnQuit();
  Terminate(MOJO_RESULT_OK);
}

}  // namespace mojo
