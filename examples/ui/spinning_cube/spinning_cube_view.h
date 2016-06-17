// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_UI_SPINNING_CUBE_SPINNING_CUBE_VIEW_H_
#define EXAMPLES_UI_SPINNING_CUBE_SPINNING_CUBE_VIEW_H_

#include <memory>

#include "examples/spinning_cube/spinning_cube.h"
#include "mojo/ui/gl_view.h"
#include "mojo/ui/input_handler.h"

namespace examples {

class SpinningCubeView : public mojo::ui::GLView,
                         public mojo::ui::InputListener {
 public:
  SpinningCubeView(
      mojo::InterfaceHandle<mojo::ApplicationConnector> app_connector,
      mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request);

  ~SpinningCubeView() override;

 private:
  // |GLView|:
  void OnDraw() override;

  // |InputListener|:
  void OnEvent(mojo::EventPtr event, const OnEventCallback& callback) override;

  void DrawCubeWithGL(const mojo::GLContext::Scope& gl_scope,
                      const mojo::Size& size);

  mojo::ui::InputHandler input_handler_;

  SpinningCube cube_;

  mojo::PointF capture_point_;
  mojo::PointF last_drag_point_;
  MojoTimeTicks drag_start_time_ = 0u;

  base::WeakPtrFactory<SpinningCubeView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SpinningCubeView);
};

}  // namespace examples

#endif  // EXAMPLES_UI_SPINNING_CUBE_SPINNING_CUBE_VIEW_H_
