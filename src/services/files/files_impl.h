// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_FILES_FILES_IMPL_H_
#define SERVICES_FILES_FILES_IMPL_H_

#include <string>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/files/interfaces/files.mojom.h"

namespace mojo {

struct ConnectionContext;

namespace files {

class FilesImpl : public Files {
 public:
  FilesImpl(const ConnectionContext& connection_context,
            InterfaceRequest<Files> request);
  ~FilesImpl() override;

  // |Files| implementation:
  // We provide a "private" temporary file system as the default. In Debug
  // builds, we also provide access to a common file system named "debug"
  // (stored under ~/MojoDebug).
  void OpenFileSystem(const mojo::String& file_system,
                      InterfaceRequest<Directory> directory,
                      const OpenFileSystemCallback& callback) override;

 private:
  const std::string client_url_;

  StrongBinding<Files> binding_;

  DISALLOW_COPY_AND_ASSIGN(FilesImpl);
};

}  // namespace files
}  // namespace mojo

#endif  // SERVICES_FILES_FILES_IMPL_H_
