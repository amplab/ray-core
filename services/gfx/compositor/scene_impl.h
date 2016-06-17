// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_GFX_COMPOSITOR_SCENE_IMPL_H_
#define SERVICES_GFX_COMPOSITOR_SCENE_IMPL_H_

#include "base/macros.h"
#include "mojo/common/binding_set.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/gfx/composition/interfaces/scenes.mojom.h"
#include "mojo/services/gfx/composition/interfaces/scheduling.mojom.h"
#include "services/gfx/compositor/compositor_engine.h"
#include "services/gfx/compositor/scene_state.h"

namespace compositor {

// Scene interface implementation.
// This object is owned by its associated SceneState.
class SceneImpl : public mojo::gfx::composition::Scene,
                  public mojo::gfx::composition::FrameScheduler {
 public:
  SceneImpl(
      CompositorEngine* engine,
      SceneState* state,
      mojo::InterfaceRequest<mojo::gfx::composition::Scene> scene_request);
  ~SceneImpl() override;

  void set_connection_error_handler(const base::Closure& handler) {
    scene_binding_.set_connection_error_handler(handler);
  }

 private:
  // |Scene|:
  void SetListener(mojo::InterfaceHandle<mojo::gfx::composition::SceneListener>
                       listener) override;
  void Update(mojo::gfx::composition::SceneUpdatePtr update) override;
  void Publish(mojo::gfx::composition::SceneMetadataPtr metadata) override;
  void GetScheduler(
      mojo::InterfaceRequest<mojo::gfx::composition::FrameScheduler>
          scheduler_request) override;

  // |FrameScheduler|:
  void ScheduleFrame(const ScheduleFrameCallback& callback) override;

  CompositorEngine* const engine_;
  SceneState* const state_;
  mojo::Binding<mojo::gfx::composition::Scene> scene_binding_;
  mojo::BindingSet<mojo::gfx::composition::FrameScheduler> scheduler_bindings_;

  DISALLOW_COPY_AND_ASSIGN(SceneImpl);
};

}  // namespace compositor

#endif  // SERVICES_GFX_COMPOSITOR_SCENE_IMPL_H_
