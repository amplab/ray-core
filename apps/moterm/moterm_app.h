// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_MOTERM_MOTERM_APP_H_
#define APPS_MOTERM_MOTERM_APP_H_

#include "base/macros.h"
#include "mojo/ui/view_provider_app.h"

class MotermApp : public mojo::ui::ViewProviderApp {
 public:
  MotermApp();
  ~MotermApp() override;

  void CreateView(
      const std::string& connection_url,
      mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
      mojo::InterfaceRequest<mojo::ServiceProvider> services) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MotermApp);
};

#endif  // APPS_MOTERM_MOTERM_APP_H_
