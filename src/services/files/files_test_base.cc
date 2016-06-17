// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/files/files_test_base.h"

#include "mojo/public/cpp/application/connect.h"
#include "mojo/services/files/interfaces/files.mojom.h"
#include "mojo/services/files/interfaces/types.mojom.h"

namespace mojo {
namespace files {

FilesTestBase::FilesTestBase() {}

FilesTestBase::~FilesTestBase() {}

void FilesTestBase::SetUp() {
  test::ApplicationTestBase::SetUp();
  ConnectToService(shell(), "mojo:files", GetSynchronousProxy(&files_));
}

void FilesTestBase::GetTemporaryRoot(
    SynchronousInterfacePtr<Directory>* directory) {
  Error error = Error::INTERNAL;
  ASSERT_TRUE(
      files_->OpenFileSystem(nullptr, GetSynchronousProxy(directory), &error));
  ASSERT_EQ(Error::OK, error);
}

void FilesTestBase::GetAppPersistentCacheRoot(
    SynchronousInterfacePtr<Directory>* directory) {
  Error error = Error::INTERNAL;
  ASSERT_TRUE(files_->OpenFileSystem("app_persistent_cache",
                                     GetSynchronousProxy(directory), &error));
  ASSERT_EQ(Error::OK, error);
}

}  // namespace files
}  // namespace mojo
