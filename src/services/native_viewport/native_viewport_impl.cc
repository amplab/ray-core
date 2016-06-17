// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_viewport/native_viewport_impl.h"

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/time/time.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/native_viewport/surface_configuration_type_converters.h"
#include "services/gles2/gpu_state.h"
#include "services/native_viewport/platform_viewport_headless.h"
#include "ui/events/event.h"
#include "ui/gl/gl_surface.h"

namespace native_viewport {

NativeViewportImpl::NativeViewportImpl(
    mojo::Shell* shell,
    bool is_headless,
    const scoped_refptr<gles2::GpuState>& gpu_state,
    mojo::InterfaceRequest<mojo::NativeViewport> request)
    : shell_(shell),
      is_headless_(is_headless),
      context_provider_(gpu_state),
      sent_metrics_(false),
      metrics_(mojo::ViewportMetrics::New()),
      binding_(this, request.Pass()),
      weak_factory_(this) {}

NativeViewportImpl::~NativeViewportImpl() {
  // Destroy the NativeViewport early on as it may call us back during
  // destruction and we want to be in a known state.
  platform_viewport_.reset();
}

void NativeViewportImpl::Create(
    mojo::SizePtr size,
    mojo::SurfaceConfigurationPtr requested_configuration,
    const CreateCallback& callback) {
  if (!requested_configuration)
    requested_configuration = mojo::SurfaceConfiguration::New();

  create_callback_ = callback;
  metrics_->size = size.Clone();
  context_provider_.set_surface_configuration(
      requested_configuration.To<gfx::SurfaceConfiguration>());
  if (is_headless_)
    platform_viewport_ = PlatformViewportHeadless::Create(this);
  else
    platform_viewport_ = PlatformViewport::Create(shell_, this);
  platform_viewport_->Init(gfx::Rect(size.To<gfx::Size>()));
}

void NativeViewportImpl::RequestMetrics(
    const RequestMetricsCallback& callback) {
  if (!sent_metrics_) {
    callback.Run(metrics_.Clone());
    sent_metrics_ = true;
    return;
  }
  metrics_callback_ = callback;
}

void NativeViewportImpl::Show() {
  platform_viewport_->Show();
}

void NativeViewportImpl::Hide() {
  platform_viewport_->Hide();
}

void NativeViewportImpl::Close() {
  DCHECK(platform_viewport_);
  platform_viewport_->Close();
}

void NativeViewportImpl::SetSize(mojo::SizePtr size) {
  platform_viewport_->SetBounds(gfx::Rect(size.To<gfx::Size>()));
}

void NativeViewportImpl::GetContextProvider(
    mojo::InterfaceRequest<mojo::ContextProvider> request) {
  context_provider_.Bind(request.Pass());
}

void NativeViewportImpl::SetEventDispatcher(
    mojo::InterfaceHandle<mojo::NativeViewportEventDispatcher> dispatcher) {
  event_dispatcher_ =
      mojo::NativeViewportEventDispatcherPtr::Create(std::move(dispatcher));
}

void NativeViewportImpl::OnMetricsChanged(mojo::ViewportMetricsPtr metrics) {
  if (metrics_->Equals(*metrics))
    return;

  metrics_ = metrics.Pass();
  sent_metrics_ = false;

  if (!metrics_callback_.is_null()) {
    metrics_callback_.Run(metrics_.Clone());
    metrics_callback_.reset();
    sent_metrics_ = true;
  }
}

void NativeViewportImpl::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget) {
  context_provider_.SetAcceleratedWidget(widget);
  // TODO: The metrics here might not match the actual window size on android
  // where we don't know the actual size until the first OnMetricsChanged call.
  create_callback_.Run(metrics_.Clone());
  sent_metrics_ = true;
  create_callback_.reset();
}

void NativeViewportImpl::OnAcceleratedWidgetDestroyed() {
  context_provider_.SetAcceleratedWidget(gfx::kNullAcceleratedWidget);
}

bool NativeViewportImpl::OnEvent(mojo::EventPtr event) {
  if (event.is_null() || !event_dispatcher_.get())
    return false;

  mojo::NativeViewportEventDispatcher::OnEventCallback callback;
  switch (event->action) {
    case mojo::EventType::POINTER_MOVE: {
      // TODO(sky): add logic to remember last event location and not send if
      // the same.
      if (pointers_waiting_on_ack_.count(event->pointer_data->pointer_id))
        return false;

      pointers_waiting_on_ack_.insert(event->pointer_data->pointer_id);
      callback =
          base::Bind(&NativeViewportImpl::AckEvent, weak_factory_.GetWeakPtr(),
                     event->pointer_data->pointer_id);
      break;
    }

    case mojo::EventType::POINTER_CANCEL:
      pointers_waiting_on_ack_.clear();
      break;

    case mojo::EventType::POINTER_UP:
      pointers_waiting_on_ack_.erase(event->pointer_data->pointer_id);
      break;

    default:
      break;
  }

  event_dispatcher_->OnEvent(event.Pass(), callback);
  return false;
}

void NativeViewportImpl::OnDestroyed() {
  delete this;
}

void NativeViewportImpl::AckEvent(int32 pointer_id) {
  pointers_waiting_on_ack_.erase(pointer_id);
}

}  // namespace native_viewport
