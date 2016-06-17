// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/drm_dmabuf_pixmap.h"

#include <i915_drm.h>
#include "base/logging.h"
#include "ui/ozone/platform/drm/gpu/drm_device.h"
#include "ui/ozone/platform/drm/gpu/drm_window.h"

namespace ui {

namespace {
void EmptyPageFlipCallback(gfx::SwapResult result) {}
}  // namespace

DrmDmabufPixmap::DrmDmabufPixmap(const scoped_refptr<DrmDevice>& drm)
    : drm_(drm) {}

DrmDmabufPixmap::~DrmDmabufPixmap() {
  // TODO: finish
  if (framebuffer_ && !drm_->RemoveFramebuffer(framebuffer_))
    PLOG(ERROR) << "SharedBuffer: RemoveFramebuffer: fb " << framebuffer_;
}

bool DrmDmabufPixmap::Initialize(base::ScopedFD dma_buf,
                                 int width,
                                 int height,
                                 uint32_t pitch) {
  int fd = dma_buf.get();
  if (!drm_->BufferFdToHandle(fd, &handle_)) {
    PLOG(ERROR) << "SharedBuffer: Initialize: couldn't get handle from fd "
                << fd;
    return false;
  }

  // For scanout
  if (!drm_->BufferHandleSetTiling(handle_, pitch, I915_TILING_X))
    return false;

  dma_buf_ = dma_buf.Pass();
  dma_buf_pitch_ = pitch;

  width_ = width;
  height_ = height;

  if (!drm_->AddFramebuffer(width_, height_, 24, 32, dma_buf_pitch_, handle_,
                            &framebuffer_)) {
    PLOG(ERROR) << "SharedBuffer: AddFramebuffer: handle " << handle_;
    return false;
  }

  return true;
}

uint32_t DrmDmabufPixmap::GetHandle() const {
  return handle_;
}

gfx::Size DrmDmabufPixmap::GetSize() const {
  return gfx::Size(width_, height_);
}

uint32_t DrmDmabufPixmap::GetFramebufferId() const {
  return framebuffer_;
}

int DrmDmabufPixmap::GetDmaBufFd() const {
  return dma_buf_.get();
}

int DrmDmabufPixmap::GetDmaBufPitch() const {
  return dma_buf_pitch_;
}

DrmDmabufPixmapWrapper::DrmDmabufPixmapWrapper(
    ScreenManager* screen_manager,
    const scoped_refptr<DrmDmabufPixmap>& pixmap)
    : pixmap_(pixmap), screen_manager_(screen_manager) {}

DrmDmabufPixmapWrapper::~DrmDmabufPixmapWrapper() {}

bool DrmDmabufPixmapWrapper::ScheduleOverlayPlane(
    gfx::AcceleratedWidget widget,
    int plane_z_order,
    gfx::OverlayTransform plane_transform,
    const gfx::Rect& display_bounds,
    const gfx::RectF& crop_rect) {
  screen_manager_->GetWindow(widget)->QueueOverlayPlane(OverlayPlane(pixmap_));
  // TODO(cstout): for the multi-overlay case, SchedulePageFlip should be called
  // once after all overlay planes are scheduled.  Tracking issue mojo #540.
  screen_manager_->GetWindow(widget)->SchedulePageFlip(
      true /* is_sync */, base::Bind(&EmptyPageFlipCallback));
  return true;
}

int DrmDmabufPixmapWrapper::GetDmaBufFd() {
  return pixmap_->GetDmaBufFd();
}

int DrmDmabufPixmapWrapper::GetDmaBufPitch() {
  return pixmap_->GetDmaBufPitch();
}

void* DrmDmabufPixmapWrapper::GetEGLClientBuffer() {
  return nullptr;
}

}  // namespace ui
