// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_GFX_COMPOSITOR_RENDERER_IMPL_H_
#define SERVICES_GFX_COMPOSITOR_RENDERER_IMPL_H_

#include "base/callback.h"
#include "base/macros.h"
#include "mojo/common/binding_set.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/gfx/composition/interfaces/renderers.mojom.h"
#include "services/gfx/compositor/compositor_engine.h"
#include "services/gfx/compositor/renderer_state.h"

namespace compositor {

// Renderer interface implementation.
// This object is owned by its associated RendererState.
class RendererImpl : public mojo::gfx::composition::Renderer,
                     public mojo::gfx::composition::FrameScheduler,
                     public mojo::gfx::composition::HitTester {
 public:
  RendererImpl(CompositorEngine* engine,
               RendererState* state,
               mojo::InterfaceRequest<mojo::gfx::composition::Renderer>
                   renderer_request);
  ~RendererImpl() override;

  void set_connection_error_handler(const base::Closure& handler) {
    renderer_binding_.set_connection_error_handler(handler);
  }

 private:
  // |Renderer|:
  void SetRootScene(mojo::gfx::composition::SceneTokenPtr scene_token,
                    uint32 scene_version,
                    mojo::RectPtr viewport) override;
  void ClearRootScene() override;
  void GetScheduler(
      mojo::InterfaceRequest<mojo::gfx::composition::FrameScheduler>
          scheduler_request) override;
  void GetHitTester(mojo::InterfaceRequest<mojo::gfx::composition::HitTester>
                        hit_tester_request) override;

  // |FrameScheduler|:
  void ScheduleFrame(const ScheduleFrameCallback& callback) override;

  // |HitTester|:
  void HitTest(mojo::PointFPtr point, const HitTestCallback& callback) override;

  CompositorEngine* const engine_;
  RendererState* const state_;
  mojo::Binding<mojo::gfx::composition::Renderer> renderer_binding_;
  mojo::BindingSet<mojo::gfx::composition::FrameScheduler> scheduler_bindings_;
  mojo::BindingSet<mojo::gfx::composition::HitTester> hit_tester_bindings;

  DISALLOW_COPY_AND_ASSIGN(RendererImpl);
};

}  // namespace compositor

#endif  // SERVICES_GFX_COMPOSITOR_RENDERER_IMPL_H_
