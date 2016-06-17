// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/ui/base_view.h"

#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "mojo/public/cpp/application/connect.h"

namespace mojo {
namespace ui {

BaseView::BaseView(InterfaceHandle<ApplicationConnector> app_connector,
                   InterfaceRequest<ViewOwner> view_owner_request,
                   const std::string& label)
    : app_connector_(ApplicationConnectorPtr::Create(app_connector.Pass())),
      view_listener_binding_(this),
      view_container_listener_binding_(this) {
  DCHECK(app_connector_);
  ConnectToService(app_connector_.get(), "mojo:view_manager_service",
                   GetProxy(&view_manager_));

  ViewListenerPtr view_listener;
  view_listener_binding_.Bind(GetProxy(&view_listener));
  view_manager_->CreateView(GetProxy(&view_), view_owner_request.Pass(),
                            view_listener.Pass(), label);
  view_->CreateScene(GetProxy(&scene_));
}

BaseView::~BaseView() {}

ServiceProvider* BaseView::GetViewServiceProvider() {
  if (!view_service_provider_)
    view_->GetServiceProvider(GetProxy(&view_service_provider_));
  return view_service_provider_.get();
}

ViewContainer* BaseView::GetViewContainer() {
  if (!view_container_) {
    view_->GetContainer(GetProxy(&view_container_));
    ViewContainerListenerPtr view_container_listener;
    view_container_listener_binding_.Bind(GetProxy(&view_container_listener));
    view_container_->SetListener(view_container_listener.Pass());
  }
  return view_container_.get();
}

mojo::gfx::composition::SceneMetadataPtr BaseView::CreateSceneMetadata() const {
  auto metadata = mojo::gfx::composition::SceneMetadata::New();
  metadata->version = scene_version_;
  metadata->presentation_time = frame_tracker_.frame_info().presentation_time;
  return metadata;
}

void BaseView::Invalidate() {
  if (!invalidated_) {
    invalidated_ = true;
    view_->Invalidate();
  }
}

void BaseView::OnPropertiesChanged(ViewPropertiesPtr old_properties) {}

void BaseView::OnLayout() {}

void BaseView::OnDraw() {}

void BaseView::OnChildAttached(uint32_t child_key,
                               ViewInfoPtr child_view_info) {}

void BaseView::OnChildUnavailable(uint32_t child_key) {}

void BaseView::OnInvalidation(ViewInvalidationPtr invalidation,
                              const OnInvalidationCallback& callback) {
  DCHECK(invalidation);
  DCHECK(invalidation->frame_info);
  TRACE_EVENT0("ui", "OnInvalidation");

  invalidated_ = false;
  frame_tracker_.Update(*invalidation->frame_info, MojoGetTimeTicksNow());
  scene_version_ = invalidation->scene_version;

  if (invalidation->properties) {
    DCHECK(invalidation->properties->display_metrics);
    DCHECK(invalidation->properties->view_layout);
    DCHECK(invalidation->properties->view_layout->size);
    TRACE_EVENT0("ui", "OnPropertiesChanged");

    ViewPropertiesPtr old_properties = std::move(properties_);
    properties_ = std::move(invalidation->properties);
    OnPropertiesChanged(std::move(old_properties));
  }

  if (!properties_)
    return;

  {
    TRACE_EVENT0("ui", "OnLayout");
    OnLayout();
  }

  if (invalidation->container_flush_token) {
    DCHECK(view_container_);  // we must have added children
    view_container_->FlushChildren(invalidation->container_flush_token);
  }

  {
    TRACE_EVENT0("ui", "OnDraw");
    OnDraw();
  }

  callback.Run();
}

void BaseView::OnChildAttached(uint32_t child_key,
                               ViewInfoPtr child_view_info,
                               const OnChildUnavailableCallback& callback) {
  DCHECK(child_view_info);

  TRACE_EVENT1("ui", "OnChildAttached", "child_key", child_key);
  OnChildAttached(child_key, child_view_info.Pass());
  callback.Run();
}

void BaseView::OnChildUnavailable(uint32_t child_key,
                                  const OnChildUnavailableCallback& callback) {
  TRACE_EVENT1("ui", "OnChildUnavailable", "child_key", child_key);
  OnChildUnavailable(child_key);
  callback.Run();
}

}  // namespace ui
}  // namespace mojo
