// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_SHADOWS_SHADOWS_VIEW_H_
#define EXAMPLES_SHADOWS_SHADOWS_VIEW_H_

#include <memory>

#include "examples/shadows/shadows_renderer.h"
#include "mojo/ui/gl_view.h"

namespace examples {

class ShadowsView : public mojo::ui::GLView {
 public:
  ShadowsView(mojo::InterfaceHandle<mojo::ApplicationConnector> app_connector,
              mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request);

  ~ShadowsView() override;

 private:
  // |GLView|:
  void OnDraw() override;

  void Render(const mojo::GLContext::Scope& gl_scope, const mojo::Size& size);

  mojo::Size size_;
  std::unique_ptr<ShadowsRenderer> renderer_;

  DISALLOW_COPY_AND_ASSIGN(ShadowsView);
};

}  // namespace examples

#endif  // EXAMPLES_SHADOWS_SHADOWS_VIEW_H_
