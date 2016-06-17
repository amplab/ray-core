// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_GFX_COMPOSITOR_RENDER_RENDER_IMAGE_H_
#define SERVICES_GFX_COMPOSITOR_RENDER_RENDER_IMAGE_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2extmojo.h>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/task_runner.h"
#include "mojo/services/gfx/composition/interfaces/resources.mojom.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace compositor {

// Describes an image which can be rendered by the compositor.
//
// Render objects are thread-safe, immutable, and reference counted.
// They have no direct references to the scene graph.
//
// TODO(jeffbrown): Generalize this beyond mailbox textures.
class RenderImage : public base::RefCountedThreadSafe<RenderImage> {
  class Releaser;
  class Generator;

 public:
  RenderImage(const sk_sp<SkImage>& image,
              const scoped_refptr<Releaser>& releaser);

  // Creates a new image backed by a mailbox texture.
  // If |sync_point| is non-zero, inserts a sync point into the command stream
  // before the image is first drawn.
  // When the last reference is released, the associated release task is
  // posted to the task runner.  Returns nullptr if the mailbox texture
  // is invalid.
  static scoped_refptr<RenderImage> CreateFromMailboxTexture(
      const GLbyte mailbox_name[GL_MAILBOX_SIZE_CHROMIUM],
      GLuint sync_point,
      uint32_t width,
      uint32_t height,
      mojo::gfx::composition::MailboxTextureResource::Origin origin,
      const scoped_refptr<base::TaskRunner>& task_runner,
      const base::Closure& release_task);

  uint32_t width() const { return image_->width(); }
  uint32_t height() const { return image_->height(); }

  // Gets the underlying image to rasterize, never null.
  const sk_sp<SkImage>& image() const { return image_; }

 private:
  friend class base::RefCountedThreadSafe<RenderImage>;

  ~RenderImage();

  sk_sp<SkImage> image_;
  scoped_refptr<Releaser> releaser_;

  DISALLOW_COPY_AND_ASSIGN(RenderImage);
};

}  // namespace compositor

#endif  // SERVICES_GFX_COMPOSITOR_RENDER_RENDER_IMAGE_H_
