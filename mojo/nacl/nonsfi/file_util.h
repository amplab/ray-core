// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_NACL_NONSFI_FILE_UTIL_H_
#define MOJO_NACL_NONSFI_FILE_UTIL_H_

#include "mojo/services/files/interfaces/files.mojom.h"
#include "mojo/tools/embed/data.h"

namespace nacl {

// Returns a file descriptor to a file descriptor holding all the data inside
// the parameter |data|. Unlinks the file behind this file descriptor, which is
// deleted when the file descriptor is closed.
int DataToTempFileDescriptor(const mojo::embed::Data& data);

// Given a pointer to an open |mojo_file|, copy the contents of the file into a
// temporary, unlinked file and return the file descriptor.
// Closes the |mojo_file| passed.
int MojoFileToTempFileDescriptor(mojo::files::FilePtr mojo_file);

// Given a file descriptor |fd| and an open |mojo_file|, copy the contents of
// the |fd| into |mojo_file|.
// Reads from |fd|, but does not close it.
// Closes the |mojo_file| passed.
void FileDescriptorToMojoFile(int fd, mojo::files::FilePtr mojo_file);

}  // namespace nacl

#endif  // MOJO_NACL_NONSFI_FILE_UTIL_H_
