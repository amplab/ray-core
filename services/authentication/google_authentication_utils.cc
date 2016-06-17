// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/authentication/google_authentication_utils.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"

namespace authentication {
namespace util {

// Mojo Shell OAuth2 Client configuration.
// TODO: These should be retrieved from a secure storage or a configuration file
// in the future.
const char* kMojoShellOAuth2ClientId =
    "962611923869-3avg0b4vlisgjhin0l98dgp6d8sd634r.apps.googleusercontent.com";
const char* kMojoShellOAuth2ClientSecret = "41IxvPPAt1HyRoYw2hO84dRI";

// Query params used in Google OAuth2 handshake
const char* kOAuth2ClientIdParamName = "client_id";
const char* kOAuth2ClientSecretParamName = "client_secret";
const char* kOAuth2ScopeParamName = "scope";
const char* kOAuth2GrantTypeParamName = "grant_type";
const char* kOAuth2CodeParamName = "code";
const char* kOAuth2RefreshTokenParamName = "refresh_token";
const char* kOAuth2DeviceFlowGrantType =
    "http://oauth.net/grant_type/device/1.0";
const char* kOAuth2RefreshTokenGrantType = "refresh_token";

// TODO(ukode) : Verify the char list
const char* kEscapableUrlParamChars = ".$[]/";

std::string EncodeParam(std::string param) {
  for (size_t i = 0; i < strlen(kEscapableUrlParamChars); ++i) {
    base::ReplaceSubstringsAfterOffset(
        &param, 0, std::string(1, kEscapableUrlParamChars[i]),
        base::StringPrintf("%%%x", kEscapableUrlParamChars[i]));
  }
  return param;
}

mojo::String BuildUrlQuery(mojo::Map<mojo::String, mojo::String> params) {
  std::string message;
  for (auto it = params.begin(); it != params.end(); ++it) {
    message += EncodeParam(it.GetKey()) + "=" + EncodeParam(it.GetValue());
    message += "&";
  }

  if (!message.empty()) {
    message = message.substr(0, message.size() - 1);  // Trims extra "&".
  }
  return message;
}

base::DictionaryValue* ParseOAuth2Response(const std::string& response) {
  if (response.empty()) {
    return nullptr;
  }

  scoped_ptr<base::Value> root(base::JSONReader::Read(response));
  if (!root || !root->IsType(base::Value::TYPE_DICTIONARY)) {
    LOG(ERROR) << "Unexpected json response:" << std::endl << response;
    return nullptr;
  }

  return static_cast<base::DictionaryValue*>(root.release());
}

}  // util namespace
}  // authentication namespace
