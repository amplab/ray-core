// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/authentication/google_authentication_impl.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/message_loop/message_loop.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "mojo/common/binding_set.h"
#include "mojo/data_pipe_utils/data_pipe_drainer.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/network/interfaces/url_loader.mojom.h"
#include "services/authentication/credentials_impl_db.mojom.h"
#include "services/authentication/google_authentication_utils.h"

using namespace authentication::util;

namespace authentication {

GoogleAuthenticationServiceImpl::GoogleAuthenticationServiceImpl(
    mojo::InterfaceRequest<AuthenticationService> request,
    const mojo::String app_url,
    mojo::NetworkServicePtr& network_service,
    mojo::files::DirectoryPtr& directory)
    : binding_(this, request.Pass()),
      app_url_(app_url),
      network_service_(network_service) {
  accounts_db_manager_ = new AccountsDbManager(directory.Pass());
}

GoogleAuthenticationServiceImpl::~GoogleAuthenticationServiceImpl() {
  delete accounts_db_manager_;
}

void GoogleAuthenticationServiceImpl::GetOAuth2Token(
    const mojo::String& username,
    mojo::Array<mojo::String> scopes,
    const GetOAuth2TokenCallback& callback) {
  if (!accounts_db_manager_->isValid()) {
    callback.Run(nullptr, "Accounts db validation failed.");
    return;
  }

  authentication::CredentialsPtr creds =
      accounts_db_manager_->GetCredentials(username);

  if (!creds->token) {
    callback.Run(nullptr, "User grant not found");
    return;
  }

  // TODO: Scopes are not used with the scoped refresh tokens. When we start
  // supporting full login scoped tokens, then the scopes here gets used for
  // Sidescoping.
  mojo::Map<mojo::String, mojo::String> params;
  params[kOAuth2ClientIdParamName] = kMojoShellOAuth2ClientId;
  params[kOAuth2ClientSecretParamName] = kMojoShellOAuth2ClientSecret;
  params[kOAuth2GrantTypeParamName] = kOAuth2RefreshTokenGrantType;
  params[kOAuth2RefreshTokenParamName] = creds->token;

  Request("https://www.googleapis.com/oauth2/v3/token", "POST",
          BuildUrlQuery(params.Pass()),
          base::Bind(&GoogleAuthenticationServiceImpl::OnGetOAuth2Token,
                     base::Unretained(this), callback));
}

void GoogleAuthenticationServiceImpl::SelectAccount(
    bool returnLastSelected,
    const SelectAccountCallback& callback) {
  if (!accounts_db_manager_->isValid()) {
    callback.Run(nullptr, "Accounts db validation failed.");
    return;
  }

  mojo::String username;
  if (returnLastSelected) {
    username = accounts_db_manager_->GetAuthorizedUserForApp(app_url_);
    if (!username.is_null()) {
      callback.Run(username, nullptr);
      return;
    }
  }

  // TODO(ukode): Select one among the list of accounts using an AccountPicker
  // UI instead of the first account always.
  mojo::Array<mojo::String> users = accounts_db_manager_->GetAllUsers();
  if (!users.size()) {
    callback.Run(nullptr, "No user accounts found.");
    return;
  }

  username = users[0];
  accounts_db_manager_->UpdateAuthorization(app_url_, username);
  callback.Run(username, nullptr);
}

void GoogleAuthenticationServiceImpl::ClearOAuth2Token(
    const mojo::String& token) {}

void GoogleAuthenticationServiceImpl::OnGetOAuth2Token(
    const GetOAuth2TokenCallback& callback,
    const std::string& response,
    const std::string& error) {
  if (response.empty()) {
    callback.Run(nullptr, "Error from server:" + error);
    return;
  }

  scoped_ptr<base::DictionaryValue> dict(ParseOAuth2Response(response.c_str()));
  if (!dict.get() || dict->HasKey("error")) {
    callback.Run(nullptr, "Error in parsing response:" + response);
    return;
  }

  std::string access_token;
  dict->GetString("access_token", &access_token);

  callback.Run(access_token, nullptr);
}

void GoogleAuthenticationServiceImpl::Request(
    const std::string& url,
    const std::string& method,
    const std::string& message,
    const mojo::Callback<void(std::string, std::string)>& callback) {
  mojo::URLRequestPtr request(mojo::URLRequest::New());
  request->url = url;
  request->method = method;
  request->auto_follow_redirects = true;

  // Add headers
  auto content_type_header = mojo::HttpHeader::New();
  content_type_header->name = "Content-Type";
  content_type_header->value = "application/x-www-form-urlencoded";
  request->headers.push_back(content_type_header.Pass());

  if (!message.empty()) {
    request->body.push_back(
        mojo::common::WriteStringToConsumerHandle(message).Pass());
  }

  mojo::URLLoaderPtr url_loader;
  network_service_->CreateURLLoader(GetProxy(&url_loader));

  url_loader->Start(
      request.Pass(),
      base::Bind(&GoogleAuthenticationServiceImpl::HandleServerResponse,
                 base::Unretained(this), callback));

  url_loader.WaitForIncomingResponse();
}

void GoogleAuthenticationServiceImpl::HandleServerResponse(
    const mojo::Callback<void(std::string, std::string)>& callback,
    mojo::URLResponsePtr response) {
  if (response.is_null()) {
    LOG(WARNING) << "Something went horribly wrong...exiting!!";
    callback.Run("", "Empty response");
    return;
  }

  if (response->error) {
    LOG(ERROR) << "Got error (" << response->error->code
               << "), reason: " << response->error->description.get().c_str();
    callback.Run("", response->error->description.get().c_str());
    return;
  }

  std::string response_body;
  mojo::common::BlockingCopyToString(response->body.Pass(), &response_body);

  callback.Run(response_body, "");
}

}  // authentication namespace
