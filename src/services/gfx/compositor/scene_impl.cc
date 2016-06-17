// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/gfx/compositor/scene_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"

namespace compositor {
namespace {
void RunScheduleFrameCallback(const SceneImpl::ScheduleFrameCallback& callback,
                              mojo::gfx::composition::FrameInfoPtr info) {
  callback.Run(info.Pass());
}
}  // namespace

SceneImpl::SceneImpl(
    CompositorEngine* engine,
    SceneState* state,
    mojo::InterfaceRequest<mojo::gfx::composition::Scene> scene_request)
    : engine_(engine),
      state_(state),
      scene_binding_(this, scene_request.Pass()) {}

SceneImpl::~SceneImpl() {}

void SceneImpl::SetListener(
    mojo::InterfaceHandle<mojo::gfx::composition::SceneListener> listener) {
  engine_->SetListener(state_, mojo::gfx::composition::SceneListenerPtr::Create(
                                   std::move(listener)));
}

void SceneImpl::Update(mojo::gfx::composition::SceneUpdatePtr update) {
  engine_->Update(state_, update.Pass());
}

void SceneImpl::Publish(mojo::gfx::composition::SceneMetadataPtr metadata) {
  engine_->Publish(state_, metadata.Pass());
}

void SceneImpl::GetScheduler(
    mojo::InterfaceRequest<mojo::gfx::composition::FrameScheduler>
        scheduler_request) {
  scheduler_bindings_.AddBinding(this, scheduler_request.Pass());
}

void SceneImpl::ScheduleFrame(const ScheduleFrameCallback& callback) {
  engine_->ScheduleFrame(state_,
                         base::Bind(RunScheduleFrameCallback, callback));
}

}  // namespace compositor
