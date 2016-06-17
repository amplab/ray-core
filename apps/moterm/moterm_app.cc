// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/moterm/moterm_app.h"

#include "apps/moterm/moterm_view.h"
#include "mojo/public/cpp/application/connect.h"

MotermApp::MotermApp() {}

MotermApp::~MotermApp() {}

void MotermApp::CreateView(
    const std::string& connection_url,
    mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
    mojo::InterfaceRequest<mojo::ServiceProvider> services) {
  new MotermView(mojo::CreateApplicationConnector(shell()),
                 view_owner_request.Pass(), services.Pass());
}
