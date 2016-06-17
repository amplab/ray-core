// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#include "services/files/files_test_base.h"

namespace mojo {
namespace files {
namespace {

using DirectoryImplTest = FilesTestBase;

TEST_F(DirectoryImplTest, Read) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  // Make some files.
  const struct {
    const char* name;
    uint32_t open_flags;
  } files_to_create[] = {
      {"my_file1", kOpenFlagRead | kOpenFlagWrite | kOpenFlagCreate},
      {"my_file2", kOpenFlagWrite | kOpenFlagCreate | kOpenFlagExclusive},
      {"my_file3", kOpenFlagWrite | kOpenFlagCreate | kOpenFlagAppend},
      {"my_file4", kOpenFlagWrite | kOpenFlagCreate | kOpenFlagTruncate}};
  for (size_t i = 0; i < arraysize(files_to_create); i++) {
    error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile(files_to_create[i].name, nullptr,
                                    files_to_create[i].open_flags, &error));
    EXPECT_EQ(Error::OK, error);
  }
  // Make a directory.
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->OpenDirectory(
      "my_dir", nullptr, kOpenFlagRead | kOpenFlagWrite | kOpenFlagCreate,
      &error));
  EXPECT_EQ(Error::OK, error);

  error = Error::INTERNAL;
  Array<DirectoryEntryPtr> directory_contents;
  ASSERT_TRUE(directory->Read(&error, &directory_contents));
  EXPECT_EQ(Error::OK, error);

  // Expected contents of the directory.
  std::map<std::string, FileType> expected_contents;
  expected_contents["my_file1"] = FileType::REGULAR_FILE;
  expected_contents["my_file2"] = FileType::REGULAR_FILE;
  expected_contents["my_file3"] = FileType::REGULAR_FILE;
  expected_contents["my_file4"] = FileType::REGULAR_FILE;
  expected_contents["my_dir"] = FileType::DIRECTORY;
  expected_contents["."] = FileType::DIRECTORY;
  expected_contents[".."] = FileType::DIRECTORY;

  EXPECT_EQ(expected_contents.size(), directory_contents.size());
  for (size_t i = 0; i < directory_contents.size(); i++) {
    ASSERT_TRUE(directory_contents[i]);
    ASSERT_TRUE(directory_contents[i]->name);
    auto it = expected_contents.find(directory_contents[i]->name.get());
    ASSERT_TRUE(it != expected_contents.end());
    EXPECT_EQ(it->second, directory_contents[i]->type);
    expected_contents.erase(it);
  }
}

// Note: Ignore nanoseconds, since it may not always be supported. We expect at
// least second-resolution support though.
// TODO(vtl): Maybe share this with |FileImplTest.StatTouch| ... but then it'd
// be harder to split this file.
TEST_F(DirectoryImplTest, StatTouch) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  // Stat it.
  error = Error::INTERNAL;
  FileInformationPtr file_info;
  ASSERT_TRUE(directory->Stat(&error, &file_info));
  EXPECT_EQ(Error::OK, error);
  ASSERT_FALSE(file_info.is_null());
  EXPECT_EQ(FileType::DIRECTORY, file_info->type);
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
  ASSERT_TRUE(directory->Touch(t.Pass(), nullptr, &error));
  EXPECT_EQ(Error::OK, error);

  // Stat again.
  error = Error::INTERNAL;
  file_info.reset();
  ASSERT_TRUE(directory->Stat(&error, &file_info));
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
  ASSERT_TRUE(directory->Touch(nullptr, t.Pass(), &error));
  EXPECT_EQ(Error::OK, error);

  // Stat again.
  error = Error::INTERNAL;
  file_info.reset();
  ASSERT_TRUE(directory->Stat(&error, &file_info));
  EXPECT_EQ(Error::OK, error);
  ASSERT_FALSE(file_info.is_null());
  ASSERT_FALSE(file_info->atime.is_null());
  EXPECT_EQ(kPartyTime1, file_info->atime->seconds);
  ASSERT_FALSE(file_info->mtime.is_null());
  EXPECT_EQ(kPartyTime2, file_info->mtime->seconds);

  // TODO(vtl): Also test Touch() "now" options.
  // TODO(vtl): Also test touching both atime and mtime.
}

// TODO(vtl): Properly test OpenFile() and OpenDirectory() (including flags).

TEST_F(DirectoryImplTest, BasicRenameDelete) {
  SynchronousInterfacePtr<Directory> directory;
  GetTemporaryRoot(&directory);
  Error error;

  // Create my_file.
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->OpenFile("my_file", nullptr,
                                  kOpenFlagWrite | kOpenFlagCreate, &error));
  EXPECT_EQ(Error::OK, error);

  // Opening my_file should succeed.
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->OpenFile("my_file", nullptr, kOpenFlagRead, &error));
  EXPECT_EQ(Error::OK, error);

  // Rename my_file to my_new_file.
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->Rename("my_file", "my_new_file", &error));
  EXPECT_EQ(Error::OK, error);

  // Opening my_file should fail.
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->OpenFile("my_file", nullptr, kOpenFlagRead, &error));
  EXPECT_EQ(Error::UNKNOWN, error);

  // Opening my_new_file should succeed.
  error = Error::INTERNAL;
  ASSERT_TRUE(
      directory->OpenFile("my_new_file", nullptr, kOpenFlagRead, &error));
  EXPECT_EQ(Error::OK, error);

  // Delete my_new_file (no flags).
  error = Error::INTERNAL;
  ASSERT_TRUE(directory->Delete("my_new_file", 0u, &error));
  EXPECT_EQ(Error::OK, error);

  // Opening my_new_file should fail.
  error = Error::INTERNAL;
  ASSERT_TRUE(
      directory->OpenFile("my_new_file", nullptr, kOpenFlagRead, &error));
  EXPECT_EQ(Error::UNKNOWN, error);
}

// TODO(vtl): Test that an open file can be moved (by someone else) without
// operations on it being affected.
// TODO(vtl): Test delete flags.

// TODO(vtl): Strictly speaking, this is a test of |FilesImpl|, but we need
// |Directory| to do anything, so it lives here.
TEST_F(DirectoryImplTest, AppPersistentCache) {
  {
    SynchronousInterfacePtr<Directory> directory;
    GetAppPersistentCacheRoot(&directory);

    // Create my_file.
    Error error = Error::INTERNAL;
    ASSERT_TRUE(directory->OpenFile("my_file", nullptr,
                                    kOpenFlagCreate | kOpenFlagWrite, &error));
    EXPECT_EQ(Error::OK, error);
  }

  {
    SynchronousInterfacePtr<Directory> directory;
    GetAppPersistentCacheRoot(&directory);

    // We should be in the same directory and my_file should still exist, so we
    // should be able to delete it.
    Error error = Error::INTERNAL;
    ASSERT_TRUE(directory->Delete("my_file", kDeleteFlagFileOnly, &error));
    EXPECT_EQ(Error::OK, error);
  }
}

}  // namespace
}  // namespace files
}  // namespace mojo
