// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/files/files_app.h"

#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/files/interfaces/files.mojom.h"
#include "services/files/files_impl.h"

namespace mojo {
namespace files {

FilesApp::FilesApp() {}
FilesApp::~FilesApp() {}

bool FilesApp::OnAcceptConnection(ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<Files>(
      [](const ConnectionContext& connection_context,
         InterfaceRequest<Files> files_request) {
        new FilesImpl(connection_context, files_request.Pass());
      });
  return true;
}

}  // namespace files
}  // namespace mojo
