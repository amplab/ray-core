// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/shadows/shadows_app.h"

#include "examples/shadows/shadows_view.h"
#include "mojo/public/cpp/application/connect.h"

namespace examples {

ShadowsApp::ShadowsApp() {}

ShadowsApp::~ShadowsApp() {}

void ShadowsApp::CreateView(
    const std::string& connection_url,
    mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
    mojo::InterfaceRequest<mojo::ServiceProvider> services) {
  new ShadowsView(mojo::CreateApplicationConnector(shell()),
                  view_owner_request.Pass());
}

}  // namespace examples
