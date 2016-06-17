// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_META_FACTORY_IMPL_H_
#define SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_META_FACTORY_IMPL_H_

#include <memory>

#include "base/callback.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/authenticating_url_loader_interceptor/interfaces/authenticating_url_loader_interceptor_meta_factory.mojom.h"
#include "mojo/services/authentication/interfaces/authentication.mojom.h"
#include "url/gurl.h"

namespace mojo {

class Shell;

class AuthenticatingURLLoaderInterceptorMetaFactoryImpl
    : public AuthenticatingURLLoaderInterceptorMetaFactory {
 public:
  AuthenticatingURLLoaderInterceptorMetaFactoryImpl(
      mojo::InterfaceRequest<AuthenticatingURLLoaderInterceptorMetaFactory>
          request,
      mojo::Shell* shell,
      std::map<GURL, std::string>* cached_tokens);
  ~AuthenticatingURLLoaderInterceptorMetaFactoryImpl() override;

 private:
  // AuthenticatingURLLoaderInterceptorMetaFactory:
  void CreateURLLoaderInterceptorFactory(
      mojo::InterfaceRequest<URLLoaderInterceptorFactory> factory_request,
      InterfaceHandle<authentication::AuthenticationService>
          authentication_service) override;

  StrongBinding<AuthenticatingURLLoaderInterceptorMetaFactory> binding_;
  Shell* const shell_;
  std::map<GURL, std::string>* cached_tokens_;
};

}  // namespace mojo

#endif  // SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_META_FACTORY_IMPL_H_
