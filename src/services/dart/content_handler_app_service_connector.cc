// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/dart/content_handler_app_service_connector.h"

#include "base/bind.h"
#include "base/location.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/services/files/interfaces/files.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"

namespace dart {

// This callback runs on the Dart content handler message loop thread. Bound
// to |this| by a weak pointer.
template<typename Interface>
void ContentHandlerAppServiceConnector::Connect(
    std::string application_name,
    mojo::InterfaceRequest<Interface> interface_request) {
  mojo::ConnectToService(shell_, application_name, interface_request.Pass());
}

ContentHandlerAppServiceConnector::ContentHandlerAppServiceConnector(
    mojo::Shell* shell)
    : runner_(base::MessageLoop::current()->task_runner()),
      shell_(shell),
      weak_ptr_factory_(this) {
  CHECK(shell != nullptr);
  CHECK(runner_.get() != nullptr);
  CHECK(runner_.get()->BelongsToCurrentThread());
}

ContentHandlerAppServiceConnector::~ContentHandlerAppServiceConnector() {
}

MojoHandle ContentHandlerAppServiceConnector::ConnectToService(
    ServiceId service_id) {
  switch (service_id) {
    case mojo::dart::DartControllerServiceConnector::kNetworkServiceId: {
      std::string application_name = "mojo:network_service";
      // Construct proxy.
      mojo::NetworkServicePtr interface_ptr;
      runner_->PostTask(FROM_HERE, base::Bind(
          &ContentHandlerAppServiceConnector::Connect<mojo::NetworkService>,
          weak_ptr_factory_.GetWeakPtr(),
          application_name,
          base::Passed(GetProxy(&interface_ptr))));
      // Return proxy end of pipe to caller.
      return interface_ptr.PassInterfaceHandle().PassHandle().release().value();
    }
    case mojo::dart::DartControllerServiceConnector::kFilesServiceId: {
      std::string application_name = "mojo:files";
      // Construct proxy.
      mojo::files::FilesPtr interface_ptr;
      runner_->PostTask(FROM_HERE, base::Bind(
          &ContentHandlerAppServiceConnector::Connect<mojo::files::Files>,
          weak_ptr_factory_.GetWeakPtr(),
          application_name,
          base::Passed(GetProxy(&interface_ptr))));
      return interface_ptr.PassInterfaceHandle().PassHandle().release().value();
    }
    break;
    default:
      return MOJO_HANDLE_INVALID;
    break;
  }
}

}  // namespace dart
