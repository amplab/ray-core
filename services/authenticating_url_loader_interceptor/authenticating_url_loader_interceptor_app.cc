// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/authenticating_url_loader_interceptor/authenticating_url_loader_interceptor_app.h"

#include "mojo/public/cpp/application/service_provider_impl.h"
#include "services/authenticating_url_loader_interceptor/authenticating_url_loader_interceptor_meta_factory_impl.h"

namespace mojo {

AuthenticatingURLLoaderInterceptorApp::AuthenticatingURLLoaderInterceptorApp() {
}

AuthenticatingURLLoaderInterceptorApp::
    ~AuthenticatingURLLoaderInterceptorApp() {}

bool AuthenticatingURLLoaderInterceptorApp::OnAcceptConnection(
    ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<
      AuthenticatingURLLoaderInterceptorMetaFactory>([this](
      const ConnectionContext& connection_context,
      InterfaceRequest<AuthenticatingURLLoaderInterceptorMetaFactory> request) {
    GURL app_url(connection_context.remote_url);
    GURL app_origin;
    if (app_url.is_valid()) {
      app_origin = app_url.GetOrigin();
    }
    new AuthenticatingURLLoaderInterceptorMetaFactoryImpl(
        request.Pass(), shell(), &tokens_[app_origin]);
  });
  return true;
}

}  // namespace mojo
