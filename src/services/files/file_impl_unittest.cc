// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/type_converter.h"
#include "mojo/services/files/interfaces/file.mojom-sync.h"
#include "services/files/files_test_base.h"

namespace mojo {
namespace files {
namespace {

using FileImplTest = FilesTestBase;

TEST_F(FileImplTest, CreateWriteCloseRenameOpenRead) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  {
    // Create my_file.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                    kOpenFlagWrite | kOpenFlagCreate, &error));
    EXPECT_EQ(Error::OK, error);

    // Write to it.
    std::vector<uint8_t> bytes_to_write;
    bytes_to_write.push_back(static_cast<uint8_t>('h'));
    bytes_to_write.push_back(static_cast<uint8_t>('e'));
    bytes_to_write.push_back(static_cast<uint8_t>('l'));
    bytes_to_write.push_back(static_cast<uint8_t>('l'));
    bytes_to_write.push_back(static_cast<uint8_t>('o'));
    error = Error::INTERNAL;
    uint32_t num_bytes_written = 0;
    ASSERT_TRUE(file->Write(Array<uint8_t>::From(bytes_to_write), 0,
                            Whence::FROM_CURRENT, &error, &num_bytes_written));
    EXPECT_EQ(Error::OK, error);
    EXPECT_EQ(bytes_to_write.size(), num_bytes_written);

    // Close it.
    error = Error::INTERNAL;
    ASSERT_TRUE(file->Close(&error));
    EXPECT_EQ(Error::OK, error);
  }

  // Rename it.
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->Rename("my_file", "your_file", &error));
  EXPECT_EQ(Error::OK, error);

  {
    // Open your_file.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile("your_file", GetSynchronousProxy(&file),
                                    kOpenFlagRead, &error));
    EXPECT_EQ(Error::OK, error);

    // Read from it.
    Array<uint8_t> bytes_read;
    error = Error::INTERNAL;
    ASSERT_TRUE(file->Read(3, 1, Whence::FROM_START, &error, &bytes_read));
    EXPECT_EQ(Error::OK, error);
    ASSERT_EQ(3u, bytes_read.size());
    EXPECT_EQ(static_cast<uint8_t>('e'), bytes_read[0]);
    EXPECT_EQ(static_cast<uint8_t>('l'), bytes_read[1]);
    EXPECT_EQ(static_cast<uint8_t>('l'), bytes_read[2]);
  }

  // TODO(vtl): Test read/write offset options.
}

TEST_F(FileImplTest, CantWriteInReadMode) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  std::vector<uint8_t> bytes_to_write;
  bytes_to_write.push_back(static_cast<uint8_t>('h'));
  bytes_to_write.push_back(static_cast<uint8_t>('e'));
  bytes_to_write.push_back(static_cast<uint8_t>('l'));
  bytes_to_write.push_back(static_cast<uint8_t>('l'));
  bytes_to_write.push_back(static_cast<uint8_t>('o'));

  {
    // Create my_file.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                    kOpenFlagWrite | kOpenFlagCreate, &error));
    EXPECT_EQ(Error::OK, error);

    // Write to it.
    error = Error::INTERNAL;
    uint32_t num_bytes_written = 0;
    ASSERT_TRUE(file->Write(Array<uint8_t>::From(bytes_to_write), 0,
                            Whence::FROM_CURRENT, &error, &num_bytes_written));
    EXPECT_EQ(Error::OK, error);
    EXPECT_EQ(bytes_to_write.size(), num_bytes_written);

    // Close it.
    error = Error::INTERNAL;
    ASSERT_TRUE(file->Close(&error));
    EXPECT_EQ(Error::OK, error);
  }

  {
    // Open my_file again, this time with read only mode.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                    kOpenFlagRead, &error));
    EXPECT_EQ(Error::OK, error);

    // Try to write in read mode; it should fail.
    error = Error::INTERNAL;
    uint32_t num_bytes_written = 0;
    ASSERT_TRUE(file->Write(Array<uint8_t>::From(bytes_to_write), 0,
                            Whence::FROM_CURRENT, &error, &num_bytes_written));
    EXPECT_EQ(Error::UNKNOWN, error);
    EXPECT_EQ(0u, num_bytes_written);

    // Close it.
    error = Error::INTERNAL;
    ASSERT_TRUE(file->Close(&error));
    EXPECT_EQ(Error::OK, error);
  }
}

TEST_F(FileImplTest, OpenExclusive) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  {
    // Create my_file.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile(
        "temp_file", GetSynchronousProxy(&file),
        kOpenFlagWrite | kOpenFlagCreate | kOpenFlagExclusive, &error));
    EXPECT_EQ(Error::OK, error);

    // Close it.
    error = Error::INTERNAL;
    ASSERT_TRUE(file->Close(&error));
    EXPECT_EQ(Error::OK, error);
  }

  {
    // Try to open my_file again in exclusive mode; it should fail.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile(
        "temp_file", GetSynchronousProxy(&file),
        kOpenFlagWrite | kOpenFlagCreate | kOpenFlagExclusive, &error));
    EXPECT_EQ(Error::UNKNOWN, error);
  }
}

TEST_F(FileImplTest, OpenInAppendMode) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  {
    // Create my_file.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                    kOpenFlagWrite | kOpenFlagCreate, &error));
    EXPECT_EQ(Error::OK, error);

    // Write to it.
    std::vector<uint8_t> bytes_to_write;
    bytes_to_write.push_back(static_cast<uint8_t>('h'));
    bytes_to_write.push_back(static_cast<uint8_t>('e'));
    bytes_to_write.push_back(static_cast<uint8_t>('l'));
    bytes_to_write.push_back(static_cast<uint8_t>('l'));
    bytes_to_write.push_back(static_cast<uint8_t>('o'));
    error = Error::INTERNAL;
    uint32_t num_bytes_written = 0;
    ASSERT_TRUE(file->Write(Array<uint8_t>::From(bytes_to_write), 0,
                            Whence::FROM_CURRENT, &error, &num_bytes_written));
    EXPECT_EQ(Error::OK, error);
    EXPECT_EQ(bytes_to_write.size(), num_bytes_written);

    // Close it.
    error = Error::INTERNAL;
    ASSERT_TRUE(file->Close(&error));
    EXPECT_EQ(Error::OK, error);
  }

  {
    // Append to my_file.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                    kOpenFlagWrite | kOpenFlagAppend, &error));
    EXPECT_EQ(Error::OK, error);

    // Write to it.
    std::vector<uint8_t> bytes_to_write;
    bytes_to_write.push_back(static_cast<uint8_t>('g'));
    bytes_to_write.push_back(static_cast<uint8_t>('o'));
    bytes_to_write.push_back(static_cast<uint8_t>('o'));
    bytes_to_write.push_back(static_cast<uint8_t>('d'));
    bytes_to_write.push_back(static_cast<uint8_t>('b'));
    bytes_to_write.push_back(static_cast<uint8_t>('y'));
    bytes_to_write.push_back(static_cast<uint8_t>('e'));
    error = Error::INTERNAL;
    uint32_t num_bytes_written = 0;
    ASSERT_TRUE(file->Write(Array<uint8_t>::From(bytes_to_write), 0,
                            Whence::FROM_CURRENT, &error, &num_bytes_written));
    EXPECT_EQ(Error::OK, error);
    EXPECT_EQ(bytes_to_write.size(), num_bytes_written);

    // Close it.
    error = Error::INTERNAL;
    ASSERT_TRUE(file->Close(&error));
    EXPECT_EQ(Error::OK, error);
  }

  {
    // Open my_file again.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                    kOpenFlagRead, &error));
    EXPECT_EQ(Error::OK, error);

    // Read from it.
    Array<uint8_t> bytes_read;
    error = Error::INTERNAL;
    ASSERT_TRUE(file->Read(12, 0, Whence::FROM_START, &error, &bytes_read));
    EXPECT_EQ(Error::OK, error);
    ASSERT_EQ(12u, bytes_read.size());
    EXPECT_EQ(static_cast<uint8_t>('l'), bytes_read[3]);
    EXPECT_EQ(static_cast<uint8_t>('o'), bytes_read[4]);
    EXPECT_EQ(static_cast<uint8_t>('g'), bytes_read[5]);
    EXPECT_EQ(static_cast<uint8_t>('o'), bytes_read[6]);
  }
}

TEST_F(FileImplTest, OpenInTruncateMode) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  {
    // Create my_file.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                    kOpenFlagWrite | kOpenFlagCreate, &error));
    EXPECT_EQ(Error::OK, error);

    // Write to it.
    std::vector<uint8_t> bytes_to_write;
    bytes_to_write.push_back(static_cast<uint8_t>('h'));
    bytes_to_write.push_back(static_cast<uint8_t>('e'));
    bytes_to_write.push_back(static_cast<uint8_t>('l'));
    bytes_to_write.push_back(static_cast<uint8_t>('l'));
    bytes_to_write.push_back(static_cast<uint8_t>('o'));
    error = Error::INTERNAL;
    uint32_t num_bytes_written = 0;
    ASSERT_TRUE(file->Write(Array<uint8_t>::From(bytes_to_write), 0,
                            Whence::FROM_CURRENT, &error, &num_bytes_written));
    EXPECT_EQ(Error::OK, error);
    EXPECT_EQ(bytes_to_write.size(), num_bytes_written);

    // Close it.
    error = Error::INTERNAL;
    ASSERT_TRUE(file->Close(&error));
    EXPECT_EQ(Error::OK, error);
  }

  {
    // Append to my_file.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                    kOpenFlagWrite | kOpenFlagTruncate,
                                    &error));
    EXPECT_EQ(Error::OK, error);

    // Write to it.
    std::vector<uint8_t> bytes_to_write;
    bytes_to_write.push_back(static_cast<uint8_t>('g'));
    bytes_to_write.push_back(static_cast<uint8_t>('o'));
    bytes_to_write.push_back(static_cast<uint8_t>('o'));
    bytes_to_write.push_back(static_cast<uint8_t>('d'));
    bytes_to_write.push_back(static_cast<uint8_t>('b'));
    bytes_to_write.push_back(static_cast<uint8_t>('y'));
    bytes_to_write.push_back(static_cast<uint8_t>('e'));
    error = Error::INTERNAL;
    uint32_t num_bytes_written = 0;
    ASSERT_TRUE(file->Write(Array<uint8_t>::From(bytes_to_write), 0,
                            Whence::FROM_CURRENT, &error, &num_bytes_written));
    EXPECT_EQ(Error::OK, error);
    EXPECT_EQ(bytes_to_write.size(), num_bytes_written);

    // Close it.
    error = Error::INTERNAL;
    ASSERT_TRUE(file->Close(&error));
    EXPECT_EQ(Error::OK, error);
  }

  {
    // Open my_file again.
    SynchronousInterfacePtr<File> file;
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                    kOpenFlagRead, &error));
    EXPECT_EQ(Error::OK, error);

    // Read from it.
    Array<uint8_t> bytes_read;
    error = Error::INTERNAL;
    ASSERT_TRUE(file->Read(7, 0, Whence::FROM_START, &error, &bytes_read));
    EXPECT_EQ(Error::OK, error);
    ASSERT_EQ(7u, bytes_read.size());
    EXPECT_EQ(static_cast<uint8_t>('g'), bytes_read[0]);
    EXPECT_EQ(static_cast<uint8_t>('o'), bytes_read[1]);
    EXPECT_EQ(static_cast<uint8_t>('o'), bytes_read[2]);
    EXPECT_EQ(static_cast<uint8_t>('d'), bytes_read[3]);
  }
}

// Note: Ignore nanoseconds, since it may not always be supported. We expect at
// least second-resolution support though.
TEST_F(FileImplTest, StatTouch) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  // Create my_file.
  SynchronousInterfacePtr<File> file;
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                  kOpenFlagWrite | kOpenFlagCreate, &error));
  EXPECT_EQ(Error::OK, error);

  // Stat it.
  error = Error::INTERNAL;
  FileInformationPtr file_info;
  ASSERT_TRUE(file->Stat(&error, &file_info));
  EXPECT_EQ(Error::OK, error);
  ASSERT_FALSE(file_info.is_null());
  EXPECT_EQ(FileType::REGULAR_FILE, file_info->type);
  EXPECT_EQ(0, file_info->size);
  ASSERT_FALSE(file_info->atime.is_null());
  EXPECT_GT(file_info->atime->seconds, 0);  // Expect that it's not 1970-01-01.
  ASSERT_FALSE(file_info->mtime.is_null());
  EXPECT_GT(file_info->mtime->seconds, 0);
  int64_t first_mtime = file_info->mtime->seconds;

  // Touch only the atime.
  error = Error::INTERNAL;
  TimespecOrNowPtr t(TimespecOrNow::New());
  t->now = false;
  t->timespec = Timespec::New();
  const int64_t kPartyTime1 = 1234567890;  // Party like it's 2009-02-13.
  t->timespec->seconds = kPartyTime1;
  ASSERT_TRUE(file->Touch(t.Pass(), nullptr, &error));
  EXPECT_EQ(Error::OK, error);

  // Stat again.
  error = Error::INTERNAL;
  file_info.reset();
  ASSERT_TRUE(file->Stat(&error, &file_info));
  EXPECT_EQ(Error::OK, error);
  ASSERT_FALSE(file_info.is_null());
  ASSERT_FALSE(file_info->atime.is_null());
  EXPECT_EQ(kPartyTime1, file_info->atime->seconds);
  ASSERT_FALSE(file_info->mtime.is_null());
  EXPECT_EQ(first_mtime, file_info->mtime->seconds);

  // Touch only the mtime.
  t = TimespecOrNow::New();
  t->now = false;
  t->timespec = Timespec::New();
  const int64_t kPartyTime2 = 1425059525;  // No time like the present.
  t->timespec->seconds = kPartyTime2;
  ASSERT_TRUE(file->Touch(nullptr, t.Pass(), &error));
  EXPECT_EQ(Error::OK, error);

  // Stat again.
  error = Error::INTERNAL;
  file_info.reset();
  ASSERT_TRUE(file->Stat(&error, &file_info));
  EXPECT_EQ(Error::OK, error);
  ASSERT_FALSE(file_info.is_null());
  ASSERT_FALSE(file_info->atime.is_null());
  EXPECT_EQ(kPartyTime1, file_info->atime->seconds);
  ASSERT_FALSE(file_info->mtime.is_null());
  EXPECT_EQ(kPartyTime2, file_info->mtime->seconds);

  // TODO(vtl): Also test non-zero file size.
  // TODO(vtl): Also test Touch() "now" options.
  // TODO(vtl): Also test touching both atime and mtime.
}

TEST_F(FileImplTest, TellSeek) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  // Create my_file.
  SynchronousInterfacePtr<File> file;
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                  kOpenFlagWrite | kOpenFlagCreate, &error));
  EXPECT_EQ(Error::OK, error);

  // Write to it.
  std::vector<uint8_t> bytes_to_write(1000, '!');
  error = Error::INTERNAL;
  uint32_t num_bytes_written = 0;
  ASSERT_TRUE(file->Write(Array<uint8_t>::From(bytes_to_write), 0,
                          Whence::FROM_CURRENT, &error, &num_bytes_written));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(bytes_to_write.size(), num_bytes_written);
  const int size = static_cast<int>(num_bytes_written);

  // Tell.
  error = Error::INTERNAL;
  int64_t position = -1;
  ASSERT_TRUE(file->Tell(&error, &position));
  // Should be at the end.
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(size, position);

  // Seek back 100.
  error = Error::INTERNAL;
  position = -1;
  ASSERT_TRUE(file->Seek(-100, Whence::FROM_CURRENT, &error, &position));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(size - 100, position);

  // Tell.
  error = Error::INTERNAL;
  position = -1;
  ASSERT_TRUE(file->Tell(&error, &position));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(size - 100, position);

  // Seek to 123 from start.
  error = Error::INTERNAL;
  position = -1;
  ASSERT_TRUE(file->Seek(123, Whence::FROM_START, &error, &position));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(123, position);

  // Tell.
  error = Error::INTERNAL;
  position = -1;
  ASSERT_TRUE(file->Tell(&error, &position));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(123, position);

  // Seek to 123 back from end.
  error = Error::INTERNAL;
  position = -1;
  ASSERT_TRUE(file->Seek(-123, Whence::FROM_END, &error, &position));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(size - 123, position);

  // Tell.
  error = Error::INTERNAL;
  position = -1;
  ASSERT_TRUE(file->Tell(&error, &position));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(size - 123, position);

  // TODO(vtl): Check that seeking actually affects reading/writing.
  // TODO(vtl): Check that seeking can extend the file?
}

TEST_F(FileImplTest, Dup) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  // Create my_file.
  SynchronousInterfacePtr<File> file1;
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->OpenFile(
      "my_file", GetSynchronousProxy(&file1),
      kOpenFlagRead | kOpenFlagWrite | kOpenFlagCreate, &error));
  EXPECT_EQ(Error::OK, error);

  // Write to it.
  std::vector<uint8_t> bytes_to_write;
  bytes_to_write.push_back(static_cast<uint8_t>('h'));
  bytes_to_write.push_back(static_cast<uint8_t>('e'));
  bytes_to_write.push_back(static_cast<uint8_t>('l'));
  bytes_to_write.push_back(static_cast<uint8_t>('l'));
  bytes_to_write.push_back(static_cast<uint8_t>('o'));
  error = Error::INTERNAL;
  uint32_t num_bytes_written = 0;
  ASSERT_TRUE(file1->Write(Array<uint8_t>::From(bytes_to_write), 0,
                           Whence::FROM_CURRENT, &error, &num_bytes_written));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(bytes_to_write.size(), num_bytes_written);
  const int end_hello_pos = static_cast<int>(num_bytes_written);

  // Dup it.
  SynchronousInterfacePtr<File> file2;
  error = Error::INTERNAL;
  ASSERT_TRUE(file1->Dup(GetSynchronousProxy(&file2), &error));
  EXPECT_EQ(Error::OK, error);

  // |file2| should have the same position.
  error = Error::INTERNAL;
  int64_t position = -1;
  ASSERT_TRUE(file2->Tell(&error, &position));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(end_hello_pos, position);

  // Write using |file2|.
  std::vector<uint8_t> more_bytes_to_write;
  more_bytes_to_write.push_back(static_cast<uint8_t>('w'));
  more_bytes_to_write.push_back(static_cast<uint8_t>('o'));
  more_bytes_to_write.push_back(static_cast<uint8_t>('r'));
  more_bytes_to_write.push_back(static_cast<uint8_t>('l'));
  more_bytes_to_write.push_back(static_cast<uint8_t>('d'));
  error = Error::INTERNAL;
  num_bytes_written = 0;
  ASSERT_TRUE(file2->Write(Array<uint8_t>::From(more_bytes_to_write), 0,
                           Whence::FROM_CURRENT, &error, &num_bytes_written));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(more_bytes_to_write.size(), num_bytes_written);
  const int end_world_pos = end_hello_pos + static_cast<int>(num_bytes_written);

  // |file1| should have the same position.
  error = Error::INTERNAL;
  position = -1;
  ASSERT_TRUE(file1->Tell(&error, &position));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(end_world_pos, position);

  // Close |file1|.
  error = Error::INTERNAL;
  ASSERT_TRUE(file1->Close(&error));
  EXPECT_EQ(Error::OK, error);

  // Read everything using |file2|.
  Array<uint8_t> bytes_read;
  error = Error::INTERNAL;
  ASSERT_TRUE(file2->Read(1000, 0, Whence::FROM_START, &error, &bytes_read));
  EXPECT_EQ(Error::OK, error);
  ASSERT_EQ(static_cast<size_t>(end_world_pos), bytes_read.size());
  // Just check the first and last bytes.
  EXPECT_EQ(static_cast<uint8_t>('h'), bytes_read[0]);
  EXPECT_EQ(static_cast<uint8_t>('d'), bytes_read[end_world_pos - 1]);

  // TODO(vtl): Test that |file2| has the same open options as |file1|.
}

TEST_F(FileImplTest, Truncate) {
  const uint32_t kInitialSize = 1000;
  const uint32_t kTruncatedSize = 654;

  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  // Create my_file.
  SynchronousInterfacePtr<File> file;
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->OpenFile("my_file", GetSynchronousProxy(&file),
                                  kOpenFlagWrite | kOpenFlagCreate, &error));
  EXPECT_EQ(Error::OK, error);

  // Write to it.
  std::vector<uint8_t> bytes_to_write(kInitialSize, '!');
  error = Error::INTERNAL;
  uint32_t num_bytes_written = 0;
  ASSERT_TRUE(file->Write(Array<uint8_t>::From(bytes_to_write), 0,
                          Whence::FROM_CURRENT, &error, &num_bytes_written));
  EXPECT_EQ(Error::OK, error);
  EXPECT_EQ(kInitialSize, num_bytes_written);

  // Stat it.
  error = Error::INTERNAL;
  FileInformationPtr file_info;
  ASSERT_TRUE(file->Stat(&error, &file_info));
  EXPECT_EQ(Error::OK, error);
  ASSERT_FALSE(file_info.is_null());
  EXPECT_EQ(kInitialSize, file_info->size);

  // Truncate it.
  error = Error::INTERNAL;
  ASSERT_TRUE(file->Truncate(kTruncatedSize, &error));
  EXPECT_EQ(Error::OK, error);

  // Stat again.
  error = Error::INTERNAL;
  file_info.reset();
  ASSERT_TRUE(file->Stat(&error, &file_info));
  EXPECT_EQ(Error::OK, error);
  ASSERT_FALSE(file_info.is_null());
  EXPECT_EQ(kTruncatedSize, file_info->size);
}

TEST_F(FileImplTest, Ioctl) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  // Create my_file.
  SynchronousInterfacePtr<File> file;
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->OpenFile(
      "my_file", GetSynchronousProxy(&file),
      kOpenFlagRead | kOpenFlagWrite | kOpenFlagCreate, &error));
  EXPECT_EQ(Error::OK, error);

  // Normal files don't support any ioctls.
  Array<uint32_t> out_values;
  ASSERT_TRUE(file->Ioctl(0, nullptr, &error, &out_values));
  EXPECT_EQ(Error::UNAVAILABLE, error);
  EXPECT_TRUE(out_values.is_null());
}

}  // namespace
}  // namespace files
}  // namespace mojo
