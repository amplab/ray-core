// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/ui/gl_view.h"

#include "base/logging.h"

namespace mojo {
namespace ui {

GLView::GLView(InterfaceHandle<ApplicationConnector> app_connector,
               InterfaceRequest<ViewOwner> view_owner_request,
               const std::string& label)
    : BaseView(app_connector.Pass(), view_owner_request.Pass(), label),
      gl_renderer_(GLContext::CreateOffscreen(BaseView::app_connector())) {}

GLView::~GLView() {}

}  // namespace ui
}  // namespace mojo
