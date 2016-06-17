// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATION_GOOGLE_AUTHENTICATION_UTILS_H_
#define SERVICES_AUTHENTICATION_GOOGLE_AUTHENTICATION_UTILS_H_

#include "base/values.h"
#include "mojo/public/cpp/bindings/map.h"
#include "mojo/public/cpp/bindings/string.h"

namespace authentication {
namespace util {

// Mojo Shell OAuth2 Client configuration.
// TODO: These should be retrieved from a secure storage or a configuration file
// in the future.
extern const char* kMojoShellOAuth2ClientId;
extern const char* kMojoShellOAuth2ClientSecret;

// Query params used in Google OAuth2 handshake
extern const char* kOAuth2ClientIdParamName;
extern const char* kOAuth2ClientSecretParamName;
extern const char* kOAuth2ScopeParamName;
extern const char* kOAuth2GrantTypeParamName;
extern const char* kOAuth2CodeParamName;
extern const char* kOAuth2RefreshTokenParamName;
extern const char* kOAuth2DeviceFlowGrantType;
extern const char* kOAuth2RefreshTokenGrantType;

extern const char* kEscapableUrlParamChars;

// Encodes a url param while calling Google OAuth endpoints over Http
std::string EncodeParam(std::string param);

// Builds a url query string from the input params
mojo::String BuildUrlQuery(mojo::Map<mojo::String, mojo::String> params);

// Parses the json response from Google's OAuth endpoint and returns the
// response in dictionary format
base::DictionaryValue* ParseOAuth2Response(const std::string& response);

}  // namespace util
}  // namespace authentication

#endif  // SERVICES_AUTHENTICATION_GOOGLE_AUTHENTICATION_UTILS_H_
