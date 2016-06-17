// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/authenticating_url_loader_interceptor/authenticating_url_loader_interceptor.h"

#include "base/bind.h"
#include "base/logging.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"

namespace mojo {

AuthenticatingURLLoaderInterceptor::AuthenticatingURLLoaderInterceptor(
    mojo::InterfaceRequest<URLLoaderInterceptor> request,
    AuthenticatingURLLoaderInterceptorFactory* factory)
    : binding_(this, request.Pass()),
      factory_(factory),
      add_authentication_(true),
      request_authorization_state_(REQUEST_INITIAL) {
}

AuthenticatingURLLoaderInterceptor::~AuthenticatingURLLoaderInterceptor() {
}

void AuthenticatingURLLoaderInterceptor::set_connection_error_handler(
    const Closure& error_handler) {
  binding_.set_connection_error_handler(error_handler);
}

void AuthenticatingURLLoaderInterceptor::InterceptRequest(
    mojo::URLRequestPtr request,
    const InterceptRequestCallback& callback) {
  // TODO(blundell): If we need to handle requests with bodies, we'll need to
  // do something here.
  if (request->body) {
    LOG(ERROR) << "Cannot pass a request to AuthenticatingURLLoaderInterceptor"
                  "that has a body";
    callback.Run(nullptr);
    return;
  }

  pending_interception_callback_ = callback;

  for (size_t i = 0; i < request->headers.size(); i++) {
    if (request->headers[i]->name == "Authorization") {
      add_authentication_ = false;
      StartRequest(request.Pass());
      return;
    }
  }

  url_ = GURL(request->url);
  auto_follow_redirects_ = request->auto_follow_redirects;
  request->auto_follow_redirects = false;
  cache_mode_ = request->cache_mode;
  headers_ = request->headers.Clone();
  std::string token = factory_->GetCachedToken(url_);
  if (token != "") {
    auto auth_header = HttpHeader::New();
    auth_header->name = "Authorization";
    auth_header->value = "Bearer " + token;
    request->headers.push_back(auth_header.Pass());
  }

  StartRequest(request.Pass());
}

void AuthenticatingURLLoaderInterceptor::InterceptFollowRedirect(
    const InterceptResponseCallback& callback) {
  if (!add_authentication_) {
    callback.Run(nullptr);
    return;
  }

  mojo::String error;

  if (!url_.is_valid()) {
    error = "No redirect to follow";
  }

  if (auto_follow_redirects_) {
    error =
        "InterceptFollowRedirect() should not be "
        "called when auto_follow_redirects has been set";
  }

  if (request_authorization_state_ != REQUEST_INITIAL) {
    error = "Not in the right state to follow a redirect";
  }

  if (!error.is_null()) {
    LOG(ERROR) << "AuthenticatingURLLoaderInterceptor: " << error;
    callback.Run(nullptr);
    return;
  }

  // If there is no cached token, have the URLLoader follow the redirect
  // itself.
  std::string token = factory_->GetCachedToken(url_);
  if (token == "") {
    callback.Run(nullptr);
    return;
  }

  pending_interception_callback_ = callback;
  StartRequest(BuildRequest(token));
}

void AuthenticatingURLLoaderInterceptor::InterceptResponse(
    mojo::URLResponsePtr response,
    const InterceptResponseCallback& callback) {
  if (!add_authentication_) {
    URLLoaderInterceptorResponsePtr interceptor_response =
        URLLoaderInterceptorResponse::New();
    interceptor_response->response = response.Pass();
    callback.Run(interceptor_response.Pass());
    return;
  }

  pending_interception_callback_ = callback;
  pending_response_ = response.Pass();

  if (pending_response_->status_code != 401 ||
      request_authorization_state_ == REQUEST_USED_FRESH_AUTH_SERVICE_TOKEN) {
    if (pending_response_->redirect_url) {
      url_ = GURL(pending_response_->redirect_url);
      request_authorization_state_ = REQUEST_INITIAL;
      if (auto_follow_redirects_) {
        // If there is no cached token, have the URLLoader follow the
        // redirect itself.
        std::string token = factory_->GetCachedToken(url_);
        if (token == "")
          pending_interception_callback_.Run(nullptr);
        else
          StartRequest(BuildRequest(token));
        pending_interception_callback_.reset();
        return;
      }
    }

    URLLoaderInterceptorResponsePtr interceptor_response =
        URLLoaderInterceptorResponse::New();
    interceptor_response->response = pending_response_.Pass();
    pending_interception_callback_.Run(interceptor_response.Pass());
    pending_interception_callback_.reset();
    return;
  }

  DCHECK(request_authorization_state_ == REQUEST_INITIAL ||
         request_authorization_state_ ==
             REQUEST_USED_CURRENT_AUTH_SERVICE_TOKEN);
  if (request_authorization_state_ == REQUEST_INITIAL) {
    request_authorization_state_ = REQUEST_USED_CURRENT_AUTH_SERVICE_TOKEN;
  } else {
    request_authorization_state_ = REQUEST_USED_FRESH_AUTH_SERVICE_TOKEN;
  }
  factory_->RetrieveToken(
      url_,
      base::Bind(&AuthenticatingURLLoaderInterceptor::OnOAuth2TokenReceived,
                 base::Unretained(this)));
}

URLRequestPtr AuthenticatingURLLoaderInterceptor::BuildRequest(
    std::string token) {
  // We should only be sending out a request with a token if the initial
  // request did not come in with an authorization header.
  DCHECK(add_authentication_);
  Array<HttpHeaderPtr> headers;
  if (headers_)
    headers = headers_.Clone();

  if (token == "")
    token = factory_->GetCachedToken(url_);

  if (token != "") {
    auto auth_header = HttpHeader::New();
    auth_header->name = "Authorization";
    auth_header->value = "Bearer " + token;
    headers.push_back(auth_header.Pass());
  }

  URLRequestPtr request(mojo::URLRequest::New());
  request->url = url_.spec();
  request->auto_follow_redirects = false;
  request->cache_mode = cache_mode_;
  request->headers = headers.Pass();

  return request;
}

void AuthenticatingURLLoaderInterceptor::StartRequest(
    mojo::URLRequestPtr request) {
  URLLoaderInterceptorResponsePtr interceptor_response =
      URLLoaderInterceptorResponse::New();
  interceptor_response->request = request.Pass();
  pending_interception_callback_.Run(interceptor_response.Pass());
  pending_interception_callback_.reset();
}

void AuthenticatingURLLoaderInterceptor::OnOAuth2TokenReceived(
    std::string token) {
  URLLoaderInterceptorResponsePtr interceptor_response =
      URLLoaderInterceptorResponse::New();

  if (token.empty()) {
    LOG(ERROR) << "Error while getting token";
    interceptor_response->response = pending_response_.Pass();
  } else {
    interceptor_response->request = BuildRequest(token);
  }

  pending_interception_callback_.Run(interceptor_response.Pass());
  pending_interception_callback_.reset();
}

}  // namespace mojo
