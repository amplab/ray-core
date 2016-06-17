// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ui/spinning_cube/spinning_cube_app.h"

#include "examples/ui/spinning_cube/spinning_cube_view.h"
#include "mojo/public/cpp/application/connect.h"

namespace examples {

SpinningCubeApp::SpinningCubeApp() {}

SpinningCubeApp::~SpinningCubeApp() {}

void SpinningCubeApp::CreateView(
    const std::string& connection_url,
    mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
    mojo::InterfaceRequest<mojo::ServiceProvider> services) {
  new SpinningCubeView(mojo::CreateApplicationConnector(shell()),
                       view_owner_request.Pass());
}

}  // namespace examples
