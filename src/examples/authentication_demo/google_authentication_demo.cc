// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/logging.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/services/authentication/interfaces/authentication.mojom.h"
#include "mojo/services/authentication/interfaces/authentication_admin.mojom.h"

namespace examples {
namespace authentication {

class GoogleAuthApp : public mojo::ApplicationImplBase {
 public:
  GoogleAuthApp() {}

  ~GoogleAuthApp() override {}

  void OnInitialize() override {
    DLOG(INFO) << "Connecting to authentication admin service...";
    mojo::ConnectToService(shell(), "mojo:authentication",
                           GetProxy(&authentication_admin_service_));
    DLOG(INFO) << "Connecting to authentication service...";
    mojo::ConnectToService(shell(), "mojo:authentication",
                           GetProxy(&authentication_service_));
    mojo::Array<mojo::String> scopes;
    scopes.push_back("profile");
    scopes.push_back("email");

    DLOG(INFO) << "Starting the device flow handshake...";
    authentication_admin_service_->GetOAuth2DeviceCode(
        scopes.Pass(),
        base::Bind(&GoogleAuthApp::OnDeviceCode, base::Unretained(this)));
  }

 private:
  void OnDeviceCode(const mojo::String& url,
                    const mojo::String& device_code,
                    const mojo::String& user_code,
                    const mojo::String& error) {
    if (!error.is_null()) {
      DLOG(INFO) << "Error: " << error;
      mojo::RunLoop::current()->Quit();  // All done!
      return;
    }
    // Display the verification url and user code in system UI and ask the
    // user to authorize in a companion device
    DLOG(INFO) << "Verification Url: " << url;
    DLOG(INFO) << "Device Code: " << device_code;
    DLOG(INFO) << "User Code: " << user_code;
    DLOG(INFO) << "Waiting for user autorization on a secondary device...";

    // Poll and exchange the user authorization to a long lived token
    DLOG(INFO) << "...";
    AddAccount(device_code);
  }

  // Exchange device code to a refresh token, and persist the grant.
  void AddAccount(const std::string device_code) {
    authentication_admin_service_->AddAccount(
        device_code,
        base::Bind(&GoogleAuthApp::OnAddAccount, base::Unretained(this)));
  }

  void OnAddAccount(const mojo::String& username, const mojo::String& error) {
    if (!error.is_null() || username.is_null()) {
      DLOG(INFO) << "Missing username or Error: " << error;
      mojo::RunLoop::current()->Quit();  // All done!
      return;
    }

    DLOG(INFO) << "Successfully registered user: " << username;
    DLOG(INFO) << "Fetching access token for user [" << username << "]...";
    FetchOAuth2AccessToken();
  }

  // Fetch a new access token for an existing user grant.
  void FetchOAuth2AccessToken() {
    authentication_service_->SelectAccount(
        true,
        base::Bind(&GoogleAuthApp::OnSelectAccount, base::Unretained(this)));
  }

  // Selects an existing user account for this app based on previous
  // authorization.
  void OnSelectAccount(const mojo::String& username,
                       const mojo::String& error) {
    if (!error.is_null()) {
      DLOG(INFO) << "Error: " << error;
      mojo::RunLoop::current()->Quit();  // All done!
      return;
    }

    DLOG(INFO) << "Selected <user> for this app: " << username;

    mojo::Array<mojo::String> scopes;
    scopes.push_back("profile");

    authentication_service_->GetOAuth2Token(
        username, scopes.Pass(),
        base::Bind(&GoogleAuthApp::OnGetOAuth2Token, base::Unretained(this)));
  }

  void OnGetOAuth2Token(const mojo::String& access_token,
                        const mojo::String& error) {
    if (!error.is_null()) {
      DLOG(INFO) << "Error: " << error;
      mojo::RunLoop::current()->Quit();  // All done!
      return;
    }

    if (access_token.is_null()) {
      DLOG(INFO) << "Unable to fetch access token, exiting!";
    } else {
      DLOG(INFO) << "Access Token: " << access_token;
    }

    DLOG(INFO) << "Fetching list of all registered users... ";
    authentication_admin_service_->GetAllUsers(
        base::Bind(&GoogleAuthApp::OnGetAllUsers, base::Unretained(this)));
  }

  // Fetches list of unique usernames for all the registered users.
  void OnGetAllUsers(const mojo::Array<mojo::String>& usernames,
                     const mojo::String& error) {
    if (!error.is_null()) {
      DLOG(INFO) << "Error: " << error;
      mojo::RunLoop::current()->Quit();  // All done!
      return;
    }

    if (usernames.size() == 0) {
      DLOG(INFO) << "No users found.";
    } else {
      for (size_t i = 0u; i < usernames.size(); ++i)
        DLOG(INFO) << "User [" << i + 1 << "]: " << usernames.at(i);
    }

    mojo::RunLoop::current()->Quit();  // All done!
    return;
  }

  ::authentication::AuthenticationServicePtr authentication_service_;
  ::authentication::AuthenticationAdminServicePtr authentication_admin_service_;
};

}  // namespace authentication
}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  examples::authentication::GoogleAuthApp google_auth_app;
  return mojo::RunApplication(application_request, &google_auth_app);
}
