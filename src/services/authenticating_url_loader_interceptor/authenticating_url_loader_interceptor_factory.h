// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_FACTORY_H_
#define SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_FACTORY_H_

#include <memory>

#include "base/callback.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/authentication/interfaces/authentication.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "url/gurl.h"

namespace mojo {

class AuthenticatingURLLoaderInterceptor;
class Shell;

class AuthenticatingURLLoaderInterceptorFactory
    : public URLLoaderInterceptorFactory {
 public:
  // TODO(vtl): Maybe this should take an
  // |InterfaceHandle<ApplicationConnector>| instead of a |Shell*|.
  AuthenticatingURLLoaderInterceptorFactory(
      mojo::InterfaceRequest<URLLoaderInterceptorFactory> request,
      mojo::InterfaceHandle<authentication::AuthenticationService>
          authentication_service,
      mojo::Shell* shell,
      std::map<GURL, std::string>* cached_tokens);
  ~AuthenticatingURLLoaderInterceptorFactory() override;

  NetworkService* network_service() { return network_service_.get(); }

  // Returns a cached token for the given url (only considers the origin). Will
  // returns an empty string if no token is cached.
  std::string GetCachedToken(const GURL& url);

  void RetrieveToken(const GURL& url,
                     const base::Callback<void(std::string)>& callback);

 private:
  // URLLoaderInterceptorFactory:
  void Create(
      mojo::InterfaceRequest<URLLoaderInterceptor> interceptor) override;

  void ClearAuthenticationService();

  void ClearInterceptor(AuthenticatingURLLoaderInterceptor* interceptor);

  void OnAccountSelected(const GURL& origin,
                         mojo::String account,
                         mojo::String error);

  void OnOAuth2TokenReceived(const GURL& origin,
                             mojo::String token,
                             mojo::String error);

  void ExecuteCallbacks(const GURL& origin, const std::string& result);

  StrongBinding<URLLoaderInterceptorFactory> binding_;
  authentication::AuthenticationServicePtr authentication_service_;
  std::map<GURL, std::string>* cached_tokens_;
  std::map<GURL, std::string> cached_accounts_;
  NetworkServicePtr network_service_;
  std::vector<std::unique_ptr<AuthenticatingURLLoaderInterceptor>>
      interceptors_;
  std::map<GURL, std::vector<base::Callback<void(std::string)>>>
      pendings_retrieve_token_;
};

}  // namespace mojo

#endif  // SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_FACTORY_H_
