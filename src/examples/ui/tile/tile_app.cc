// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_split.h"
#include "examples/ui/tile/tile_app.h"
#include "examples/ui/tile/tile_view.h"
#include "mojo/public/cpp/application/connect.h"
#include "url/third_party/mozilla/url_parse.h"

namespace examples {

TileApp::TileApp() {}

TileApp::~TileApp() {}

void TileApp::CreateView(
    const std::string& connection_url,
    mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
    mojo::InterfaceRequest<mojo::ServiceProvider> services) {
  TileParams params;
  if (!ParseParams(connection_url, &params)) {
    LOG(ERROR) << "Missing or invalid URL parameters.  See README.";
    return;
  }

  new TileView(mojo::CreateApplicationConnector(shell()),
               view_owner_request.Pass(), params);
}

bool TileApp::ParseParams(const std::string& connection_url,
                          TileParams* params) {
  url::Parsed parsed;
  url::ParseStandardURL(connection_url.c_str(), connection_url.size(), &parsed);
  url::Component query = parsed.query;

  for (;;) {
    url::Component key, value;
    if (!url::ExtractQueryKeyValue(connection_url.c_str(), &query, &key,
                                   &value))
      break;
    std::string key_str(connection_url, key.begin, key.len);
    std::string value_str(connection_url, value.begin, value.len);
    if (key_str == "views") {
      params->view_urls = base::SplitString(
          value_str, ",", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    } else if (key_str == "vm") {
      if (value_str == "any")
        params->version_mode = TileParams::VersionMode::kAny;
      else if (value_str == "exact")
        params->version_mode = TileParams::VersionMode::kExact;
      else
        return false;
    } else if (key_str == "cm") {
      if (value_str == "merge")
        params->combinator_mode = TileParams::CombinatorMode::kMerge;
      else if (value_str == "prune")
        params->combinator_mode = TileParams::CombinatorMode::kPrune;
      else if (value_str == "flash")
        params->combinator_mode = TileParams::CombinatorMode::kFallbackFlash;
      else if (value_str == "dim")
        params->combinator_mode = TileParams::CombinatorMode::kFallbackDim;
      else
        return false;
    } else if (key_str == "o") {
      if (value_str == "h")
        params->orientation_mode = TileParams::OrientationMode::kHorizontal;
      else if (value_str == "v")
        params->orientation_mode = TileParams::OrientationMode::kVertical;
      else
        return false;
    } else {
      return false;
    }
  }

  return !params->view_urls.empty();
}

}  // namespace examples
