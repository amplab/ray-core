// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_DRM_DMABUF_PIXMAP_H_
#define UI_OZONE_PLATFORM_DRM_GPU_DRM_DMABUF_PIXMAP_H_

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "ui/ozone/platform/drm/gpu/scanout_buffer.h"
#include "ui/ozone/platform/drm/gpu/screen_manager.h"
#include "ui/ozone/public/native_pixmap.h"

namespace ui {

class DrmDevice;

// DrmDmabufPixmap is a reference to a dmabuf file descriptor.
// 'Pixmap' not 'buffer' because we have a size (width and height).
class DrmDmabufPixmap : public ScanoutBuffer {
 public:
  DrmDmabufPixmap(const scoped_refptr<DrmDevice>& drm);
  ~DrmDmabufPixmap() override;

  bool Initialize(base::ScopedFD dma_buf,
                  int width,
                  int height,
                  uint32_t pitch);

  // ScanoutBuffer:
  uint32_t GetFramebufferId() const override;
  uint32_t GetHandle() const override;
  gfx::Size GetSize() const override;

  int GetDmaBufFd() const;
  int GetDmaBufPitch() const;

 private:
  scoped_refptr<DrmDevice> drm_;

  // PRIME file descriptor.
  base::ScopedFD dma_buf_;
  uint32_t dma_buf_pitch_ = -1;

  // Local gem handle for the file descriptior.
  uint32_t handle_ = 0;

  // Framebuffer ID for scanout.
  uint32_t framebuffer_ = 0;

  int width_ = 0;
  int height_ = 0;
};

class DrmDmabufPixmapWrapper : public NativePixmap {
 public:
  DrmDmabufPixmapWrapper(ScreenManager* screen_manager,
                         const scoped_refptr<DrmDmabufPixmap>& pixmap);
  ~DrmDmabufPixmapWrapper() override;

  // NativePixmap
  void* /* EGLClientBuffer */ GetEGLClientBuffer() override;
  int GetDmaBufFd() override;
  int GetDmaBufPitch() override;

  bool ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                            int plane_z_order,
                            gfx::OverlayTransform plane_transform,
                            const gfx::Rect& display_bounds,
                            const gfx::RectF& crop_rect) override;

 private:
  scoped_refptr<DrmDmabufPixmap> pixmap_;
  ScreenManager* screen_manager_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_DRM_DMABUF_PIXMAP_H_
