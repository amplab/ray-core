// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/ui/ganesh_view.h"

#include "base/logging.h"
#include "mojo/skia/ganesh_texture_surface.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace mojo {
namespace ui {

GaneshView::GaneshView(InterfaceHandle<ApplicationConnector> app_connector,
                       InterfaceRequest<ViewOwner> view_owner_request,
                       const std::string& label)
    : BaseView(app_connector.Pass(), view_owner_request.Pass(), label),
      ganesh_renderer_(new skia::GaneshContext(
          GLContext::CreateOffscreen(BaseView::app_connector()))) {}

GaneshView::~GaneshView() {}

}  // namespace ui
}  // namespace mojo
