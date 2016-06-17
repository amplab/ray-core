// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_UI_NOODLES_NOODLES_APP_H_
#define EXAMPLES_UI_NOODLES_NOODLES_APP_H_

#include "mojo/ui/view_provider_app.h"

namespace examples {

class NoodlesApp : public mojo::ui::ViewProviderApp {
 public:
  NoodlesApp();
  ~NoodlesApp() override;

  void CreateView(
      const std::string& connection_url,
      mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
      mojo::InterfaceRequest<mojo::ServiceProvider> services) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NoodlesApp);
};

}  // namespace examples

#endif  // EXAMPLES_UI_NOODLES_NOODLES_APP_H_
