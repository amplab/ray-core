// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/nacl/nonsfi/file_util.h"

#include <fcntl.h>
#include <unistd.h>

#include <memory>
#include <vector>

#include "base/files/file_util.h"
#include "mojo/services/files/interfaces/files.mojom.h"
#include "mojo/tools/embed/data.h"

namespace nacl {

int DataToTempFileDescriptor(const mojo::embed::Data& data) {
  base::FilePath path;
  CHECK(base::CreateTemporaryFile(&path))
      << "Could not create temp file for data";

  int fd = open(path.value().c_str(), O_RDWR);
  CHECK_GE(fd, 0) << "Could not open temporary file";

  size_t bytes_left_to_write = data.size;
  const char* source = data.data;
  while (bytes_left_to_write > 0) {
    ssize_t bytes = HANDLE_EINTR(write(fd, source, bytes_left_to_write));
    CHECK_GE(bytes, 0) << "Error writing data to temp file";
    bytes_left_to_write -= bytes;
    source += bytes;
  }

  CHECK(!unlink(path.value().c_str())) << "Could not unlink temporary file";
  return fd;
}

int MojoFileToTempFileDescriptor(mojo::files::FilePtr mojo_file) {
  base::FilePath path;
  CHECK(base::CreateTemporaryFile(&path))
      << "Could not create temp file for data";

  int fd = open(path.value().c_str(), O_RDWR);
  CHECK_GE(fd, 0) << "Could not open temporary file";
  CHECK(!unlink(path.value().c_str())) << "Could not unlink temporary file";

  mojo::files::Error error = mojo::files::Error::INTERNAL;
  mojo::files::FileInformationPtr file_info;
  mojo_file->Stat([&error, &file_info](mojo::files::Error e,
                                       mojo::files::FileInformationPtr f) {
    error = e;
    file_info = f.Pass();
  });
  CHECK(mojo_file.WaitForIncomingResponse());
  CHECK_EQ(mojo::files::Error::OK, error);
  CHECK(!file_info.is_null());

  size_t bytes_left_to_write = file_info->size;
  int64_t offset = 0;
  static const size_t kBufferSize = 0x100000;
  while (bytes_left_to_write > 0) {
    size_t bytes_to_read = kBufferSize;
    if (bytes_left_to_write < kBufferSize)
      bytes_to_read = bytes_left_to_write;
    mojo::Array<uint8_t> buf;
    mojo_file->Read(
        bytes_to_read, offset, mojo::files::Whence::FROM_START,
        [&error, &buf](mojo::files::Error e, mojo::Array<uint8_t> b) {
          error = e;
          buf = b.Pass();
        });
    CHECK(mojo_file.WaitForIncomingResponse());
    ssize_t bytes = HANDLE_EINTR(write(fd, buf.data(), bytes_to_read));
    CHECK_GE(bytes, 0) << "Error writing data to temp file";
    bytes_left_to_write -= bytes;
    offset += bytes;
  }

  // Close the mojo file supplied
  mojo_file->Close([&error](mojo::files::Error e) { error = e; });
  CHECK(mojo_file.WaitForIncomingResponse());
  CHECK_EQ(mojo::files::Error::OK, error);

  return fd;
}

void FileDescriptorToMojoFile(int fd, mojo::files::FilePtr mojo_file) {
  mojo::files::Error error = mojo::files::Error::INTERNAL;

  static const size_t kBufferSize = 0x100000;
  std::unique_ptr<uint8_t[]> buf(new uint8_t[kBufferSize]);
  int64_t offset = 0;
  while (true) {
    ssize_t bytes = HANDLE_EINTR(read(fd, buf.get(), kBufferSize));
    CHECK_GE(bytes, 0);
    if (bytes == 0)
      break;
    std::vector<uint8_t> source_v;
    source_v.assign(buf.get(), buf.get() + bytes);
    ssize_t num_bytes_written = 0;
    mojo_file->Write(
        mojo::Array<uint8_t>::From(source_v), offset,
        mojo::files::Whence::FROM_START,
        [&error, &num_bytes_written](mojo::files::Error e, ssize_t n) {
          error = e;
          num_bytes_written = n;
        });
    CHECK(mojo_file.WaitForIncomingResponse());
    CHECK_EQ(mojo::files::Error::OK, error);
    CHECK_EQ(num_bytes_written, bytes);
    offset += bytes;
  }

  mojo_file->Close([&error](mojo::files::Error e) { error = e; });
  CHECK(mojo_file.WaitForIncomingResponse());
  CHECK_EQ(mojo::files::Error::OK, error);
}

}  // namespace nacl
