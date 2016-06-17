// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ui/noodles/noodles_app.h"

#include "examples/ui/noodles/noodles_view.h"
#include "mojo/public/cpp/application/connect.h"

namespace examples {

NoodlesApp::NoodlesApp() {}

NoodlesApp::~NoodlesApp() {}

void NoodlesApp::CreateView(
    const std::string& connection_url,
    mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
    mojo::InterfaceRequest<mojo::ServiceProvider> services) {
  new NoodlesView(mojo::CreateApplicationConnector(shell()),
                  view_owner_request.Pass());
}

}  // namespace examples
