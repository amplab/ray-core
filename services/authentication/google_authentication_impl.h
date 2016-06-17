// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATION_GOOGLE_AUTHENTICATION_IMPL_H_
#define SERVICES_AUTHENTICATION_GOOGLE_AUTHENTICATION_IMPL_H_

#include "mojo/common/binding_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/authentication/interfaces/authentication.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "services/authentication/accounts_db_manager.h"

namespace authentication {

// Implementation of AuthenticationService from
// services/authentication/authentication.mojom for Google users.
class GoogleAuthenticationServiceImpl
    : public authentication::AuthenticationService {
 public:
  GoogleAuthenticationServiceImpl(
      mojo::InterfaceRequest<AuthenticationService> request,
      const mojo::String app_url,
      mojo::NetworkServicePtr& network_service,
      mojo::files::DirectoryPtr& directory);

  ~GoogleAuthenticationServiceImpl() override;

  void GetOAuth2Token(const mojo::String& username,
                      mojo::Array<mojo::String> scopes,
                      const GetOAuth2TokenCallback& callback) override;

  void SelectAccount(bool returnLastSelected,
                     const SelectAccountCallback& callback) override;

  void ClearOAuth2Token(const mojo::String& token) override;

 private:
  void OnGetOAuth2Token(const GetOAuth2TokenCallback& callback,
                        const std::string& response,
                        const std::string& error);

  // Makes a Http request to the server endpoint
  void Request(const std::string& url,
               const std::string& method,
               const std::string& message,
               const mojo::Callback<void(std::string, std::string)>& callback);

  // Handles Http response from the server
  void HandleServerResponse(
      const mojo::Callback<void(std::string, std::string)>& callback,
      mojo::URLResponsePtr response);

  mojo::StrongBinding<AuthenticationService> binding_;
  std::string app_url_;
  mojo::NetworkServicePtr& network_service_;
  AccountsDbManager* accounts_db_manager_;

  DISALLOW_COPY_AND_ASSIGN(GoogleAuthenticationServiceImpl);
};

}  // namespace authentication

#endif  // SERVICES_AUTHENTICATION_GOOGLE_AUTHENTICATION_IMPL_H_
