// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_FILES_FILES_TEST_BASE_H_
#define SERVICES_FILES_FILES_TEST_BASE_H_

#include "base/macros.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "mojo/services/files/interfaces/directory.mojom-sync.h"
#include "mojo/services/files/interfaces/files.mojom-sync.h"

namespace mojo {
namespace files {

class FilesTestBase : public test::ApplicationTestBase {
 public:
  FilesTestBase();
  ~FilesTestBase() override;

  void SetUp() override;

 protected:
  // Note: These have out parameters rather than returning
  // |SynchronousInterfacePtr<Directory>|, since |ASSERT_...()| doesn't work
  // with return values.
  void GetTemporaryRoot(SynchronousInterfacePtr<Directory>* directory);
  void GetAppPersistentCacheRoot(SynchronousInterfacePtr<Directory>* directory);

 private:
  SynchronousInterfacePtr<Files> files_;

  DISALLOW_COPY_AND_ASSIGN(FilesTestBase);
};

}  // namespace files
}  // namespace mojo

#endif  // SERVICES_FILES_FILES_TEST_BASE_H_
