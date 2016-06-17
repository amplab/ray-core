// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/gfx/compositor/backend/gpu_rasterizer.h"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2extmojo.h>
#include <MGL/mgl.h>
#include <MGL/mgl_echo.h>
#include <MGL/mgl_onscreen.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "services/gfx/compositor/render/render_frame.h"

namespace compositor {
namespace {
// Timeout for receiving initial viewport parameters from the GPU service.
constexpr int64_t kViewportParameterTimeoutMs = 1000;

// Default vsync interval when the GPU service failed to provide viewport
// parameters promptly.
constexpr int64_t kDefaultVsyncIntervalUs = 100000;  // deliberately sluggish
}

GpuRasterizer::GpuRasterizer(mojo::ContextProviderPtr context_provider,
                             Callbacks* callbacks)
    : context_provider_(context_provider.Pass()),
      callbacks_(callbacks),
      viewport_parameter_listener_binding_(this),
      viewport_parameter_timeout_(false, false),
      weak_ptr_factory_(this) {
  DCHECK(context_provider_);
  DCHECK(callbacks_);

  context_provider_.set_connection_error_handler(
      base::Bind(&GpuRasterizer::OnContextProviderConnectionError,
                 base::Unretained(this)));
  CreateContext();
}

GpuRasterizer::~GpuRasterizer() {
  DestroyContext();
}

void GpuRasterizer::CreateContext() {
  DCHECK(!gl_context_);

  have_viewport_parameters_ = false;

  mojo::ViewportParameterListenerPtr viewport_parameter_listener;
  viewport_parameter_listener_binding_.Bind(
      GetProxy(&viewport_parameter_listener));
  context_provider_->Create(
      viewport_parameter_listener.Pass(),
      base::Bind(&GpuRasterizer::InitContext, base::Unretained(this)));
}

void GpuRasterizer::InitContext(
    mojo::InterfaceHandle<mojo::CommandBuffer> command_buffer) {
  DCHECK(!gl_context_);
  DCHECK(!ganesh_context_);
  DCHECK(!ganesh_surface_);

  if (!command_buffer) {
    LOG(ERROR) << "Could not create GL context.";
    callbacks_->OnRasterizerError();
    return;
  }

  gl_context_ = mojo::GLContext::CreateFromCommandBuffer(
      mojo::CommandBufferPtr::Create(std::move(command_buffer)));
  DCHECK(!gl_context_->is_lost());
  gl_context_->AddObserver(this);
  ganesh_context_ = new mojo::skia::GaneshContext(gl_context_);

  if (have_viewport_parameters_) {
    ApplyViewportParameters();
  } else {
    viewport_parameter_timeout_.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(kViewportParameterTimeoutMs),
        base::Bind(&GpuRasterizer::OnViewportParameterTimeout,
                   base::Unretained(this)));
  }
}

void GpuRasterizer::AbandonContext() {
  if (viewport_parameter_listener_binding_.is_bound()) {
    viewport_parameter_timeout_.Stop();
    viewport_parameter_listener_binding_.Close();
  }

  if (ready_) {
    while (frames_in_progress_)
      DrawFinished(false /*presented*/);
    ready_ = false;
    callbacks_->OnRasterizerSuspended();
  }
}

void GpuRasterizer::DestroyContext() {
  AbandonContext();

  if (gl_context_) {
    ganesh_context_ = nullptr;
    gl_context_ = nullptr;

    // Do this after releasing the GL context so that we will already have
    // told the Ganesh context to abandon its context.
    ganesh_surface_.reset();
  }
}

void GpuRasterizer::OnContextProviderConnectionError() {
  LOG(ERROR) << "Context provider connection lost.";

  callbacks_->OnRasterizerError();
}

void GpuRasterizer::OnContextLost() {
  LOG(WARNING) << "GL context lost!";

  AbandonContext();
  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(&GpuRasterizer::RecreateContextAfterLoss,
                            weak_ptr_factory_.GetWeakPtr()));
}

void GpuRasterizer::RecreateContextAfterLoss() {
  LOG(WARNING) << "Recreating GL context.";

  DestroyContext();
  CreateContext();
}

void GpuRasterizer::OnViewportParameterTimeout() {
  DCHECK(!have_viewport_parameters_);

  LOG(WARNING) << "Viewport parameter listener timeout after "
               << kViewportParameterTimeoutMs << " ms: assuming "
               << kDefaultVsyncIntervalUs
               << " us vsync interval, rendering will be janky!";

  OnVSyncParametersUpdated(0, kDefaultVsyncIntervalUs);
}

void GpuRasterizer::OnVSyncParametersUpdated(int64_t timebase,
                                             int64_t interval) {
  DVLOG(1) << "Vsync parameters: timebase=" << timebase
           << ", interval=" << interval;

  if (!have_viewport_parameters_) {
    viewport_parameter_timeout_.Stop();
    have_viewport_parameters_ = true;
  }
  vsync_timebase_ = timebase;
  vsync_interval_ = interval;
  ApplyViewportParameters();
}

void GpuRasterizer::ApplyViewportParameters() {
  DCHECK(have_viewport_parameters_);

  if (gl_context_ && !gl_context_->is_lost()) {
    ready_ = true;
    callbacks_->OnRasterizerReady(vsync_timebase_, vsync_interval_);
  }
}

void GpuRasterizer::DrawFrame(const scoped_refptr<RenderFrame>& frame) {
  DCHECK(frame);
  DCHECK(ready_);
  DCHECK(gl_context_);
  DCHECK(!gl_context_->is_lost());
  DCHECK(ganesh_context_);

  uint32_t frame_number = total_frames_++;
  frames_in_progress_++;
  TRACE_EVENT1("gfx", "GpuRasterizer::DrawFrame", "num", frame_number);

  mojo::GLContext::Scope gl_scope(gl_context_);

  // Update the viewport.
  const SkIRect& viewport = frame->viewport();
  bool stale_surface = false;
  if (!ganesh_surface_ ||
      ganesh_surface_->surface()->width() != viewport.width() ||
      ganesh_surface_->surface()->height() != viewport.height()) {
    glResizeCHROMIUM(viewport.width(), viewport.height(), 1.0f);
    glViewport(viewport.x(), viewport.y(), viewport.width(), viewport.height());
    stale_surface = true;
  }

  // Draw the frame content.
  {
    mojo::skia::GaneshContext::Scope ganesh_scope(ganesh_context_);

    if (stale_surface) {
      ganesh_surface_.reset(
          new mojo::skia::GaneshFramebufferSurface(ganesh_scope));
    }

    frame->Draw(ganesh_surface_->canvas());
  }

  // Swap buffers.
  {
    TRACE_EVENT0("gfx", "MGLSwapBuffers");
    MGLSwapBuffers();
  }

  // Listen for completion.
  TRACE_EVENT_ASYNC_BEGIN0("gfx", "MGLEcho", frame_number);
  MGLEcho(&GpuRasterizer::OnMGLEchoReply, this);
}

void GpuRasterizer::DrawFinished(bool presented) {
  DCHECK(frames_in_progress_);

  uint32_t frame_number = total_frames_ - frames_in_progress_;
  frames_in_progress_--;
  TRACE_EVENT2("gfx", "GpuRasterizer::DrawFinished", "num", frame_number,
               "presented", presented);
  TRACE_EVENT_ASYNC_END0("gfx", "MGLEcho", frame_number);

  callbacks_->OnRasterizerFinishedDraw(presented);
}

void GpuRasterizer::OnMGLEchoReply(void* context) {
  auto rasterizer = static_cast<GpuRasterizer*>(context);
  if (rasterizer->ready_)
    rasterizer->DrawFinished(true /*presented*/);
}

}  // namespace compositor
