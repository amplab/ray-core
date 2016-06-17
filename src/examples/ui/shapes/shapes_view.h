// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_UI_SHAPES_SHAPES_VIEW_H_
#define EXAMPLES_UI_SHAPES_SHAPES_VIEW_H_

#include "mojo/ui/ganesh_view.h"

class SkCanvas;

namespace examples {

class ShapesView : public mojo::ui::GaneshView {
 public:
  ShapesView(mojo::InterfaceHandle<mojo::ApplicationConnector> app_connector,
             mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request);

  ~ShapesView() override;

 private:
  // |GaneshView|:
  void OnDraw() override;

  void DrawContent(const mojo::skia::GaneshContext::Scope& ganesh_scope,
                   const mojo::Size& size,
                   SkCanvas* canvas);

  DISALLOW_COPY_AND_ASSIGN(ShapesView);
};

}  // namespace examples

#endif  // EXAMPLES_UI_SHAPES_SHAPES_VIEW_H_
