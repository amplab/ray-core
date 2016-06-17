// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_UI_NOODLES_RASTERIZER_H_
#define EXAMPLES_UI_NOODLES_RASTERIZER_H_

#include <memory>

#include "mojo/public/interfaces/application/application_connector.mojom.h"
#include "mojo/services/gfx/composition/interfaces/scenes.mojom.h"
#include "mojo/ui/ganesh_renderer.h"

namespace examples {

class Frame;

// Ganesh-based rasterizer which runs on a separate thread from the view.
// Calls into this object, including its creation, must be posted to the
// correct message loop by the view.
class Rasterizer {
 public:
  Rasterizer(mojo::ApplicationConnectorPtr connector,
             mojo::gfx::composition::ScenePtr scene);

  ~Rasterizer();

  void PublishFrame(std::unique_ptr<Frame> frame);

 private:
  mojo::ui::GaneshRenderer ganesh_renderer_;
  mojo::gfx::composition::ScenePtr scene_;

  DISALLOW_COPY_AND_ASSIGN(Rasterizer);
};

}  // namespace examples

#endif  // EXAMPLES_UI_NOODLES_RASTERIZER_H_
