// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_GFX_COMPOSITOR_COMPOSITOR_IMPL_H_
#define SERVICES_GFX_COMPOSITOR_COMPOSITOR_IMPL_H_

#include "base/macros.h"
#include "mojo/common/strong_binding_set.h"
#include "mojo/services/gfx/composition/interfaces/compositor.mojom.h"
#include "services/gfx/compositor/compositor_engine.h"

namespace compositor {

// Compositor interface implementation.
class CompositorImpl : public mojo::gfx::composition::Compositor {
 public:
  explicit CompositorImpl(CompositorEngine* engine);
  ~CompositorImpl() override;

 private:
  // |Compositor|:
  void CreateScene(
      mojo::InterfaceRequest<mojo::gfx::composition::Scene> scene_request,
      const mojo::String& label,
      const CreateSceneCallback& callback) override;
  void CreateRenderer(
      mojo::InterfaceHandle<mojo::ContextProvider> context_provider,
      mojo::InterfaceRequest<mojo::gfx::composition::Renderer> renderer_request,
      const mojo::String& label) override;

  CompositorEngine* engine_;

  DISALLOW_COPY_AND_ASSIGN(CompositorImpl);
};

}  // namespace compositor

#endif  // SERVICES_GFX_COMPOSITOR_COMPOSITOR_IMPL_H_
