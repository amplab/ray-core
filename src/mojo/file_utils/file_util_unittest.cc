// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <string.h>

#include <utility>
#include <vector>

#include "mojo/file_utils/file_util.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/services/files/interfaces/directory.mojom-sync.h"
#include "mojo/services/files/interfaces/file.mojom-sync.h"
#include "mojo/services/files/interfaces/files.mojom-sync.h"

namespace file_utils {
namespace test {
namespace {

using FileUtilTest = mojo::test::ApplicationTestBase;

// Writes |input| to |file| and that the requisite number of bytes was written.
void WriteFileHelper(mojo::SynchronousInterfacePtr<mojo::files::File>* file,
                     const std::string& input) {
  auto bytes_to_write = mojo::Array<uint8_t>::New(input.size());
  memcpy(bytes_to_write.data(), input.data(), input.size());
  mojo::files::Error error = mojo::files::Error::INTERNAL;
  uint32_t num_bytes_written = 0;
  ASSERT_TRUE((*file)->Write(std::move(bytes_to_write), 0,
                             mojo::files::Whence::FROM_CURRENT, &error,
                             &num_bytes_written));
  EXPECT_EQ(mojo::files::Error::OK, error);
  EXPECT_EQ(input.size(), num_bytes_written);
}

// Seeks to |offset| in |file| (from the current position), verifying that the
// resulting position is |expected_position|.
void SeekFileHelper(mojo::SynchronousInterfacePtr<mojo::files::File>* file,
                    int64_t offset,
                    int64_t expected_position) {
  int64_t position = -1;
  mojo::files::Error error = mojo::files::Error::INTERNAL;
  ASSERT_TRUE((*file)->Seek(offset, mojo::files::Whence::FROM_CURRENT, &error,
                            &position));
  EXPECT_EQ(mojo::files::Error::OK, error);
  EXPECT_EQ(expected_position, position);
}

// Reads |expected_output.size()| bytes from |file| at offset |offset| (using
// Whence::FROM_CURRENT), and verify that the result matches |expected_output|.
void ReadFileHelper(mojo::SynchronousInterfacePtr<mojo::files::File>* file,
                    int64_t offset,
                    const std::string& expected_output) {
  mojo::Array<uint8_t> bytes_read;
  mojo::files::Error error = mojo::files::Error::INTERNAL;
  uint32_t num_bytes_to_read = expected_output.size();
  ASSERT_TRUE((*file)->Read(num_bytes_to_read, offset,
                            mojo::files::Whence::FROM_CURRENT, &error,
                            &bytes_read));
  EXPECT_EQ(mojo::files::Error::OK, error);
  ASSERT_EQ(num_bytes_to_read, bytes_read.size());
  EXPECT_EQ(
      0, memcmp(bytes_read.data(), expected_output.data(), num_bytes_to_read));
}

// Closes |file| and verifies that no error occurs.
void CloseFileHelper(mojo::SynchronousInterfacePtr<mojo::files::File>* file) {
  mojo::files::Error error = mojo::files::Error::INTERNAL;
  ASSERT_TRUE((*file)->Close(&error));
  EXPECT_EQ(mojo::files::Error::OK, error);
}

// Opens the file named |path_name| in |directory| and verifies that no error
// occurs. Returns the newly-opened file.
mojo::SynchronousInterfacePtr<mojo::files::File> OpenFileHelper(
    mojo::SynchronousInterfacePtr<mojo::files::Directory>* directory,
    const std::string& path_name) {
  mojo::SynchronousInterfacePtr<mojo::files::File> temp_file;
  mojo::files::Error error = mojo::files::Error::INTERNAL;
  EXPECT_TRUE(
      (*directory)
          ->OpenFile(path_name, GetSynchronousProxy(&temp_file),
                     mojo::files::kOpenFlagWrite | mojo::files::kOpenFlagRead,
                     &error));
  EXPECT_EQ(mojo::files::Error::OK, error);
  return temp_file;
}

TEST_F(FileUtilTest, BasicCreateTemporaryFile) {
  mojo::SynchronousInterfacePtr<mojo::files::Files> files;
  mojo::ConnectToService(shell(), "mojo:files", GetSynchronousProxy(&files));

  mojo::files::Error error = mojo::files::Error::INTERNAL;
  mojo::SynchronousInterfacePtr<mojo::files::Directory> directory;
  ASSERT_TRUE(files->OpenFileSystem(
      nullptr, mojo::GetSynchronousProxy(&directory), &error));
  EXPECT_EQ(mojo::files::Error::OK, error);

  std::string filename1;
  auto file1 = mojo::SynchronousInterfacePtr<mojo::files::File>::Create(
      CreateTemporaryFileInDir(&directory, &filename1));
  ASSERT_TRUE(file1);
  std::string filename2;
  auto file2 = mojo::SynchronousInterfacePtr<mojo::files::File>::Create(
      CreateTemporaryFileInDir(&directory, &filename2));
  ASSERT_TRUE(file2);
  std::string filename3;
  auto file3 = mojo::SynchronousInterfacePtr<mojo::files::File>::Create(
      CreateTemporaryFileInDir(&directory, &filename3));
  ASSERT_TRUE(file3);

  // The temp filenames should not be equal.
  EXPECT_NE(filename1, filename2);
  EXPECT_NE(filename1, filename3);
  EXPECT_NE(filename2, filename3);

  // Test that 'Write' can be called on the temp files.
  WriteFileHelper(&file1, "abcde");
  WriteFileHelper(&file2, "fghij");
  WriteFileHelper(&file3, "lmnop");

  // Test that 'Seek' can be called on the temp files.
  SeekFileHelper(&file1, -5, 0);
  SeekFileHelper(&file2, -4, 1);
  SeekFileHelper(&file3, -3, 2);

  // Test that 'Read' can be called on the temp files.
  ReadFileHelper(&file1, 0, "abcde");
  ReadFileHelper(&file2, 0, "ghij");
  ReadFileHelper(&file3, 0, "nop");

  // Test that the files can be closed.
  CloseFileHelper(&file1);
  CloseFileHelper(&file2);
  CloseFileHelper(&file3);

  // Test that the files can be reopened after closing.
  file1 = OpenFileHelper(&directory, filename1);
  file2 = OpenFileHelper(&directory, filename2);
  file3 = OpenFileHelper(&directory, filename3);

  // Verify the contents of the reopened files.
  ReadFileHelper(&file1, 0, "abcde");
  ReadFileHelper(&file2, 0, "fghij");
  ReadFileHelper(&file3, 0, "lmnop");

  // Test that the files can be closed once more.
  CloseFileHelper(&file1);
  CloseFileHelper(&file2);
  CloseFileHelper(&file3);
}

}  // namespace
}  // namespace test
}  // namespace file_utils
