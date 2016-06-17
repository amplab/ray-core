// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains utility functions around the mojo files service.

#ifndef MOJO_FILE_UTILS_FILE_UTIL_H_
#define MOJO_FILE_UTILS_FILE_UTIL_H_

#include <string>

#include "mojo/public/cpp/bindings/interface_handle.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "mojo/services/files/interfaces/directory.mojom-sync.h"
#include "mojo/services/files/interfaces/file.mojom.h"

namespace file_utils {

// Creates a new temp file, placed in the (input) directory |directory| along
// with the (output) file location in |path_name|. This function blocks the
// calling thread, and there must not be any calls pending on the |directory|
// when this function is called.
// Returns a FilePtr on success, and null on failure.
// On failure, |path_name| is unchanged.
mojo::InterfaceHandle<mojo::files::File> CreateTemporaryFileInDir(
    mojo::SynchronousInterfacePtr<mojo::files::Directory>* directory,
    std::string* path_name);

}  // namespace file_utils

#endif  // MOJO_FILE_UTILS_FILE_UTIL_H_
