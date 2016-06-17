// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/file_utils/file_util.h"

#include <stdint.h>

#include "mojo/services/files/interfaces/types.mojom.h"

namespace file_utils {

mojo::InterfaceHandle<mojo::files::File> CreateTemporaryFileInDir(
    mojo::SynchronousInterfacePtr<mojo::files::Directory>* directory,
    std::string* path_name) {
  std::string internal_path_name;
  const std::string charset("abcdefghijklmnopqrstuvwxyz1234567890");
  const unsigned kSuffixLength = 6;
  const unsigned kMaxNumAttempts = 10;
  mojo::InterfaceHandle<mojo::files::File> temp_file;
  mojo::files::Error error = mojo::files::Error::INTERNAL;
  uint64_t random_value = 0;
  for (unsigned attempt = 0; attempt < kMaxNumAttempts; attempt++) {
    internal_path_name = ".org.chromium.Mojo.";
    uint64_t microseconds = static_cast<uint64_t>(MojoGetTimeTicksNow());
    for (unsigned i = 0; i < kSuffixLength; i++) {
      // This roughly matches glibc's mapping from time to "random_time_bits",
      // it seems to be good enough for creating temporary files.
      random_value += (microseconds << 16) ^ (microseconds / 1000000);
      internal_path_name.push_back(charset[random_value % charset.length()]);
    }
    if (!(*directory)
             ->OpenFile(internal_path_name, GetProxy(&temp_file),
                        mojo::files::kOpenFlagWrite |
                            mojo::files::kOpenFlagRead |
                            mojo::files::kOpenFlagCreate |
                            mojo::files::kOpenFlagExclusive,
                        &error))
      return nullptr;
    // TODO(smklein): React to error code when ErrnoToError can return something
    // other than Error::UNKNOWN. We only want to retry when "EEXIST" is thrown.
    if (error == mojo::files::Error::OK) {
      *path_name = internal_path_name;
      return temp_file;
    }
  }
  return nullptr;
}

}  // namespace file_utils
