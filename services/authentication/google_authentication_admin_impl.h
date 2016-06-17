// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATION_GOOGLE_AUTHENTICATION_ADMIN_IMPL_H_
#define SERVICES_AUTHENTICATION_GOOGLE_AUTHENTICATION_ADMIN_IMPL_H_

#include "mojo/common/binding_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/authentication/interfaces/authentication_admin.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "services/authentication/accounts_db_manager.h"

namespace authentication {

// Implementation of AuthenticationService from
// services/authentication/authentication_admin.mojom for Google users.
class GoogleAuthenticationAdminServiceImpl
    : public authentication::AuthenticationAdminService {
 public:
  GoogleAuthenticationAdminServiceImpl(
      mojo::InterfaceRequest<AuthenticationAdminService> admin_request,
      const mojo::String app_url,
      mojo::NetworkServicePtr& network_service,
      mojo::files::DirectoryPtr& directory);

  ~GoogleAuthenticationAdminServiceImpl() override;

  void GetOAuth2DeviceCode(
      mojo::Array<mojo::String> scopes,
      const GetOAuth2DeviceCodeCallback& callback) override;

  void AddAccount(const mojo::String& device_code,
                  const AddAccountCallback& callback) override;

  void GetAllUsers(const GetAllUsersCallback& callback) override;

 private:
  // Polls recursively for user grant authorized on secondary device and
  // on success, adds the user account to the credentials database and returns
  // the username. On error, an error description is returned.
  void AddAccountInternal(const mojo::String& device_code,
                          const uint32_t num_poll_attempts,
                          const AddAccountCallback& callback);

  void OnGetOAuth2DeviceCode(const GetOAuth2DeviceCodeCallback& callback,
                             const std::string& response,
                             const std::string& error);

  void OnAddAccount(const AddAccountCallback& callback,
                    const mojo::String& device_code,
                    const uint32_t num_poll_attempts,
                    const std::string& response,
                    const std::string& error);

  // Fetches token info from access token.
  void GetTokenInfo(const std::string& access_token);

  void OnGetTokenInfo(const std::string& response, const std::string& error);

  // Fetches user info from id token.
  void GetUserInfo(const std::string& id_token);

  void OnGetUserInfo(const std::string& response, const std::string& error);

  // Makes a Http request to the server endpoint
  void Request(const std::string& url,
               const std::string& method,
               const std::string& message,
               const mojo::Callback<void(std::string, std::string)>& callback);

  void Request(const std::string& url,
               const std::string& method,
               const std::string& message,
               const mojo::Callback<void(std::string, std::string)>& callback,
               const mojo::String& device_code,
               const uint32_t num_poll_attempts);

  // Handles Http response from the server
  void HandleServerResponse(
      const mojo::Callback<void(std::string, std::string)>& callback,
      const mojo::String& device_code,
      const uint32_t num_poll_attempts,
      mojo::URLResponsePtr response);

  std::string user_id_;
  std::string email_;
  std::string scope_;
  mojo::StrongBinding<AuthenticationAdminService> binding_;
  std::string app_url_;
  mojo::NetworkServicePtr& network_service_;
  AccountsDbManager* accounts_db_manager_;

  DISALLOW_COPY_AND_ASSIGN(GoogleAuthenticationAdminServiceImpl);
};

}  // namespace authentication

#endif  // SERVICES_AUTHENTICATION_GOOGLE_AUTHENTICATION_IMPL_H_
