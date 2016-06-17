// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/application_manager/fetcher.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "url/gurl.h"

namespace shell {

namespace {
const char kMojoMagic[] = "#!mojo ";
const size_t kMaxShebangLength = 2048;
}

Fetcher::Fetcher(const FetchCallback& loader_callback)
    : loader_callback_(loader_callback) {
}

Fetcher::~Fetcher() {
}

bool Fetcher::PeekContentHandler(std::string* mojo_shebang,
                                 GURL* mojo_content_handler_url) {
  // TODO(aa): I guess this should just go in ApplicationManager now.
  std::string shebang;
  if (HasMojoMagic() && PeekFirstLine(&shebang)) {
    GURL url(shebang.substr(arraysize(kMojoMagic) - 1, std::string::npos));
    if (url.is_valid()) {
      *mojo_shebang = shebang;
      *mojo_content_handler_url = url;
      return true;
    }
  }
  return false;
}

// static
bool Fetcher::HasMojoMagic(const base::FilePath& path) {
  DCHECK(!path.empty());
  std::string magic;
  ReadFileToString(path, &magic, strlen(kMojoMagic));
  return magic == kMojoMagic;
}

// static
bool Fetcher::PeekFirstLine(const base::FilePath& path, std::string* line) {
  DCHECK(!path.empty());
  std::string start_of_file;
  ReadFileToString(path, &start_of_file, kMaxShebangLength);
  size_t return_position = start_of_file.find('\n');
  if (return_position == std::string::npos)
    return false;
  *line = start_of_file.substr(0, return_position + 1);
  return true;
}

}  // namespace shell
