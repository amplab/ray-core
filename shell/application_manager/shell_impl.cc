// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/application_manager/shell_impl.h"

#include <utility>

#include "mojo/converters/url/url_type_converters.h"
#include "mojo/services/content_handler/interfaces/content_handler.mojom.h"
#include "shell/application_manager/application_manager.h"

using mojo::ApplicationConnector;
using mojo::ApplicationPtr;
using mojo::Array;
using mojo::InterfaceRequest;
using mojo::InterfaceHandle;
using mojo::ServiceProvider;
using mojo::ServiceProviderPtr;
using mojo::Shell;
using mojo::ShellPtr;
using mojo::String;

namespace shell {

// ShellImpl::ApplicationConnectorImpl -----------------------------------------

ShellImpl::ApplicationConnectorImpl::ApplicationConnectorImpl(Shell* shell)
    : shell_(shell) {}

ShellImpl::ApplicationConnectorImpl::~ApplicationConnectorImpl() {}

void ShellImpl::ApplicationConnectorImpl::ConnectToApplication(
    const String& app_url,
    InterfaceRequest<ServiceProvider> services) {
  shell_->ConnectToApplication(app_url, std::move(services));
}

void ShellImpl::ApplicationConnectorImpl::Duplicate(
    InterfaceRequest<ApplicationConnector> application_connector_request) {
  bindings_.AddBinding(this, std::move(application_connector_request));
}

// ShellImpl -------------------------------------------------------------------

ShellImpl::ShellImpl(ApplicationPtr application,
                     ApplicationManager* manager,
                     const Identity& identity,
                     const base::Closure& on_application_end)
    : manager_(manager),
      identity_(identity),
      on_application_end_(on_application_end),
      application_(std::move(application)),
      binding_(this),
      application_connector_impl_(this) {
  binding_.set_connection_error_handler(
      [this]() { manager_->OnShellImplError(this); });
}

ShellImpl::~ShellImpl() {
}

void ShellImpl::InitializeApplication(Array<String> args) {
  ShellPtr shell;
  binding_.Bind(GetProxy(&shell));
  application_->Initialize(std::move(shell), std::move(args),
                           identity_.url.spec());
}

void ShellImpl::ConnectToClient(const GURL& requested_url,
                                const GURL& requestor_url,
                                InterfaceRequest<ServiceProvider> services) {
  application_->AcceptConnection(String::From(requestor_url),
                                 requested_url.spec(), std::move(services));
}

void ShellImpl::ConnectToApplication(
    const String& app_url,
    InterfaceRequest<ServiceProvider> services) {
  GURL app_gurl(app_url);
  if (!app_gurl.is_valid()) {
    LOG(ERROR) << "Error: invalid URL: " << app_url;
    return;
  }
  manager_->ConnectToApplication(app_gurl, identity_.url, std::move(services),
                                 base::Closure());
}

void ShellImpl::CreateApplicationConnector(
    InterfaceRequest<ApplicationConnector> application_connector_request) {
  application_connector_impl_.Duplicate(
      std::move(application_connector_request));
}

}  // namespace shell
