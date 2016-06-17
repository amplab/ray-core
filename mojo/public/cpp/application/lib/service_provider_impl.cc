// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/service_provider_impl.h"

#include <utility>

namespace mojo {

ServiceProviderImpl::ServiceProviderImpl()
    : binding_(this), fallback_service_provider_(nullptr) {
}

ServiceProviderImpl::ServiceProviderImpl(
    const ConnectionContext& connection_context,
    InterfaceRequest<ServiceProvider> service_provider_request)
    : binding_(this), fallback_service_provider_(nullptr) {
  if (service_provider_request.is_pending())
    Bind(connection_context, service_provider_request.Pass());
}

ServiceProviderImpl::~ServiceProviderImpl() {}

void ServiceProviderImpl::Bind(
    const ConnectionContext& connection_context,
    InterfaceRequest<ServiceProvider> service_provider_request) {
  connection_context_ = connection_context;
  binding_.Bind(service_provider_request.Pass());
}

void ServiceProviderImpl::Close() {
  if (binding_.is_bound()) {
    binding_.Close();
    connection_context_ = ConnectionContext();
  }
}

void ServiceProviderImpl::AddServiceForName(
    std::unique_ptr<ServiceConnector> service_connector,
    const std::string& service_name) {
  name_to_service_connector_[service_name] = std::move(service_connector);
}

void ServiceProviderImpl::RemoveServiceForName(
    const std::string& service_name) {
  auto it = name_to_service_connector_.find(service_name);
  if (it != name_to_service_connector_.end())
    name_to_service_connector_.erase(it);
}

void ServiceProviderImpl::ConnectToService(
    const String& service_name,
    ScopedMessagePipeHandle client_handle) {
  auto it = name_to_service_connector_.find(service_name);
  if (it != name_to_service_connector_.end()) {
    it->second->ConnectToService(connection_context_, client_handle.Pass());
  } else if (fallback_service_provider_) {
    fallback_service_provider_->ConnectToService(service_name,
                                                 client_handle.Pass());
  }
}

}  // namespace mojo
