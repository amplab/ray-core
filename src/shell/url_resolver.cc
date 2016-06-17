// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/url_resolver.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "shell/application_manager/query_util.h"
#include "shell/filename_util.h"
#include "shell/switches.h"
#include "url/url_util.h"

namespace shell {

URLResolver::URLResolver() {
  // Needed to treat first component of mojo URLs as host, not path.
  url::AddStandardScheme("mojo");
}

URLResolver::~URLResolver() {
}

// static
std::vector<URLResolver::OriginMapping> URLResolver::GetOriginMappings(
    const std::vector<std::string> args) {
  std::vector<OriginMapping> origin_mappings;
  const std::string kArgsForSwitches[] = {
      "-" + std::string(switches::kMapOrigin) + "=",
      "--" + std::string(switches::kMapOrigin) + "=",
  };
  for (auto& arg : args) {
    for (size_t i = 0; i < arraysize(kArgsForSwitches); i++) {
      const std::string& argsfor_switch = kArgsForSwitches[i];
      if (arg.compare(0, argsfor_switch.size(), argsfor_switch) == 0) {
        std::string value =
            arg.substr(argsfor_switch.size(), std::string::npos);
        size_t delim = value.find('=');
        if (delim <= 0 || delim >= value.size())
          continue;
        origin_mappings.push_back(
            OriginMapping(value.substr(0, delim),
                          value.substr(delim + 1, std::string::npos)));
      }
    }
  }
  return origin_mappings;
}

void URLResolver::AddURLMapping(const GURL& url, const GURL& mapped_url) {
  url_map_[url] = mapped_url;
}

void URLResolver::AddOriginMapping(const GURL& origin, const GURL& base_url) {
  if (!origin.is_valid() || !base_url.is_valid() ||
      origin != origin.GetOrigin()) {
    // Disallow invalid mappings.
    LOG(ERROR) << "Invalid origin for mapping: " << origin;
    return;
  }
  // Force both origin and base_url to have trailing slashes.
  origin_map_[origin] = AddTrailingSlashIfNeeded(base_url);
}

GURL URLResolver::ApplyMappings(const GURL& url) const {
  std::string query;
  GURL mapped_url = GetBaseURLAndQuery(url, &query);
  for (;;) {
    const auto& url_it = url_map_.find(mapped_url);
    if (url_it != url_map_.end()) {
      mapped_url = url_it->second;
      continue;
    }

    GURL origin = mapped_url.GetOrigin();
    const auto& origin_it = origin_map_.find(origin);
    if (origin_it == origin_map_.end())
      break;
    mapped_url = GURL(origin_it->second.spec() +
                      mapped_url.spec().substr(origin.spec().length()));
  }

  if (query.length())
    mapped_url = GURL(mapped_url.spec() + query);
  return mapped_url;
}

void URLResolver::SetMojoBaseURL(const GURL& mojo_base_url) {
  DCHECK(mojo_base_url.is_valid());
  // Force a trailing slash on the base_url to simplify resolving
  // relative files and URLs below.
  mojo_base_url_ = AddTrailingSlashIfNeeded(mojo_base_url);
}

GURL URLResolver::ResolveMojoURL(const GURL& mojo_url) const {
  if (mojo_url.scheme() != "mojo") {
    // The mapping has produced some sort of non-mojo: URL - file:, http:, etc.
    return mojo_url;
  } else {
    // It's still a mojo: URL, use the default mapping scheme.
    std::string query;
    GURL base_url = GetBaseURLAndQuery(mojo_url, &query);
    std::string lib = base_url.host() + ".mojo" + query;
    return mojo_base_url_.Resolve(lib);
  }
}

}  // namespace shell
