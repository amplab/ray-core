// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/authenticating_url_loader_interceptor/authenticating_url_loader_interceptor_factory.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "mojo/public/cpp/application/connect.h"
#include "services/authenticating_url_loader_interceptor/authenticating_url_loader_interceptor.h"

namespace mojo {

AuthenticatingURLLoaderInterceptorFactory::
    AuthenticatingURLLoaderInterceptorFactory(
        mojo::InterfaceRequest<URLLoaderInterceptorFactory> request,
        mojo::InterfaceHandle<authentication::AuthenticationService>
            authentication_service,
        mojo::Shell* shell,
        std::map<GURL, std::string>* cached_tokens)
    : binding_(this, request.Pass()),
      authentication_service_(authentication::AuthenticationServicePtr::Create(
          std::move(authentication_service))),
      cached_tokens_(cached_tokens) {
  ConnectToService(shell, "mojo:network_service", GetProxy(&network_service_));
  authentication_service_.set_connection_error_handler(
      [this]() { ClearAuthenticationService(); });
}

AuthenticatingURLLoaderInterceptorFactory::
    ~AuthenticatingURLLoaderInterceptorFactory() {
}

std::string AuthenticatingURLLoaderInterceptorFactory::GetCachedToken(
    const GURL& url) {
  GURL origin = url.GetOrigin();
  if (cached_tokens_->find(origin) != cached_tokens_->end()) {
    return (*cached_tokens_)[origin];
  }
  return "";
}

void AuthenticatingURLLoaderInterceptorFactory::RetrieveToken(
    const GURL& url,
    const base::Callback<void(std::string)>& callback) {
  if (!authentication_service_) {
    callback.Run("");
    return;
  }
  GURL origin = url.GetOrigin();
  bool request_in_flight =
      (pendings_retrieve_token_.find(origin) != pendings_retrieve_token_.end());
  pendings_retrieve_token_[origin].push_back(callback);

  if (request_in_flight)
    return;

  // Initiate a request to retrieve the token.
  if (cached_tokens_->find(origin) != cached_tokens_->end()) {
    // Clear the cached token in case the request is due to that token being
    // stale.
    authentication_service_->ClearOAuth2Token((*cached_tokens_)[origin]);
    cached_tokens_->erase(origin);
  }
  if (cached_accounts_.find(origin) != cached_accounts_.end()) {
    OnAccountSelected(origin, cached_accounts_[origin], mojo::String());
  } else {
    authentication_service_->SelectAccount(
        true, base::Bind(
                  &AuthenticatingURLLoaderInterceptorFactory::OnAccountSelected,
                  base::Unretained(this), origin));
  }
}

void AuthenticatingURLLoaderInterceptorFactory::ClearInterceptor(
    AuthenticatingURLLoaderInterceptor* interceptor) {
  auto it = std::find_if(
      interceptors_.begin(), interceptors_.end(),
      [interceptor](
          const std::unique_ptr<AuthenticatingURLLoaderInterceptor>& p) {
        return p.get() == interceptor;
      });
  DCHECK(it != interceptors_.end());
  interceptors_.erase(it);
}

void AuthenticatingURLLoaderInterceptorFactory::ClearAuthenticationService() {
  authentication_service_ = nullptr;

  // All pending requests need to fail.
  for (const auto& callbacks : pendings_retrieve_token_) {
    for (const auto& callback : callbacks.second) {
      callback.Run("");
    }
  }
  pendings_retrieve_token_.clear();
}

void AuthenticatingURLLoaderInterceptorFactory::Create(
    mojo::InterfaceRequest<URLLoaderInterceptor> interceptor) {
  std::unique_ptr<AuthenticatingURLLoaderInterceptor> interceptor_impl(
      new AuthenticatingURLLoaderInterceptor(interceptor.Pass(), this));
  interceptor_impl->set_connection_error_handler(base::Bind(
      &AuthenticatingURLLoaderInterceptorFactory::ClearInterceptor,
      base::Unretained(this), base::Unretained(interceptor_impl.get())));
  interceptors_.push_back(std::move(interceptor_impl));
}

void AuthenticatingURLLoaderInterceptorFactory::OnAccountSelected(
    const GURL& origin,
    mojo::String account,
    mojo::String error) {
  DCHECK(authentication_service_);
  if (error) {
    LOG(WARNING) << "Error (" << error << ") while selecting account";
    ExecuteCallbacks(origin, "");
    return;
  }
  cached_accounts_[origin] = account;
  auto scopes = mojo::Array<mojo::String>::New(1);
  scopes[0] = "https://www.googleapis.com/auth/userinfo.email";
  authentication_service_->GetOAuth2Token(
      account, scopes.Pass(),
      base::Bind(
          &AuthenticatingURLLoaderInterceptorFactory::OnOAuth2TokenReceived,
          base::Unretained(this), origin));
}

void AuthenticatingURLLoaderInterceptorFactory::OnOAuth2TokenReceived(
    const GURL& origin,
    mojo::String token,
    mojo::String error) {
  if (error) {
    LOG(WARNING) << "Error (" << error << ") while getting token";
    ExecuteCallbacks(origin, "");
    return;
  }
  std::string string_token(token);
  (*cached_tokens_)[origin] = string_token;
  ExecuteCallbacks(origin, string_token);
}

void AuthenticatingURLLoaderInterceptorFactory::ExecuteCallbacks(
    const GURL& origin,
    const std::string& result) {
  for (auto& callback : pendings_retrieve_token_[origin]) {
    callback.Run(result);
  }
  pendings_retrieve_token_.erase(origin);
}

}  // namespace mojo
