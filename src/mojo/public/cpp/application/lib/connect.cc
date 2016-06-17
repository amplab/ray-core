// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/connect.h"

#include "mojo/public/interfaces/application/shell.mojom.h"

namespace mojo {

InterfaceHandle<ApplicationConnector> CreateApplicationConnector(Shell* shell) {
  InterfaceHandle<ApplicationConnector> application_connector;
  shell->CreateApplicationConnector(GetProxy(&application_connector));
  return application_connector;
}

InterfaceHandle<ApplicationConnector> DuplicateApplicationConnector(
    ApplicationConnector* application_connector) {
  InterfaceHandle<ApplicationConnector> new_application_connector;
  application_connector->Duplicate(GetProxy(&new_application_connector));
  return new_application_connector;
}

}  // namespace mojo
