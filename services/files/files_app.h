// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_FILES_FILES_APP_H_
#define SERVICES_FILES_FILES_APP_H_

#include "base/macros.h"
#include "mojo/public/cpp/application/application_impl_base.h"

namespace mojo {
namespace files {

class FilesApp : public ApplicationImplBase {
 public:
  FilesApp();
  ~FilesApp() override;

 private:
  // |ApplicationImplBase| override:
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override;

  DISALLOW_COPY_AND_ASSIGN(FilesApp);
};

}  // namespace files
}  // namespace mojo

#endif  // SERVICES_FILES_FILES_APP_H_
