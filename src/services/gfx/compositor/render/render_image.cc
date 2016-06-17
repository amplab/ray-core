// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/gfx/compositor/render/render_image.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "mojo/skia/ganesh_image_factory.h"
#include "third_party/skia/include/core/SkImage.h"

namespace compositor {

// Invokes the release callback when both the Generator and the RenderImage
// have been freed.  Note that the Generator may outlive the RenderImage.
class RenderImage::Releaser
    : public base::RefCountedThreadSafe<RenderImage::Releaser> {
 public:
  Releaser(const scoped_refptr<base::TaskRunner>& task_runner,
           const base::Closure& release_task)
      : task_runner_(task_runner), release_task_(release_task) {
    DCHECK(task_runner_);
  }

 private:
  friend class base::RefCountedThreadSafe<RenderImage::Releaser>;

  ~Releaser() { task_runner_->PostTask(FROM_HERE, release_task_); }

  scoped_refptr<base::TaskRunner> const task_runner_;
  base::Closure const release_task_;
};

class RenderImage::Generator : public mojo::skia::MailboxTextureImageGenerator {
 public:
  Generator(const scoped_refptr<Releaser>& releaser,
            const GLbyte mailbox_name[GL_MAILBOX_SIZE_CHROMIUM],
            GLuint sync_point,
            uint32_t width,
            uint32_t height,
            GrSurfaceOrigin origin)
      : MailboxTextureImageGenerator(mailbox_name,
                                     sync_point,
                                     width,
                                     height,
                                     origin),
        releaser_(releaser) {
    DCHECK(releaser_);
  }

  ~Generator() override {}

 private:
  scoped_refptr<Releaser> releaser_;
};

RenderImage::RenderImage(const sk_sp<SkImage>& image,
                         const scoped_refptr<Releaser>& releaser)
    : image_(image), releaser_(releaser) {
  DCHECK(image_);
  DCHECK(releaser_);
}

RenderImage::~RenderImage() {}

scoped_refptr<RenderImage> RenderImage::CreateFromMailboxTexture(
    const GLbyte mailbox_name[GL_MAILBOX_SIZE_CHROMIUM],
    GLuint sync_point,
    uint32_t width,
    uint32_t height,
    mojo::gfx::composition::MailboxTextureResource::Origin origin,
    const scoped_refptr<base::TaskRunner>& task_runner,
    const base::Closure& release_task) {
  scoped_refptr<Releaser> releaser = new Releaser(task_runner, release_task);
  sk_sp<SkImage> image = SkImage::MakeFromGenerator(
      new Generator(releaser, mailbox_name, sync_point, width, height,
                    origin == mojo::gfx::composition::MailboxTextureResource::
                                  Origin::BOTTOM_LEFT
                        ? kBottomLeft_GrSurfaceOrigin
                        : kTopLeft_GrSurfaceOrigin));
  if (!image)
    return nullptr;

  return new RenderImage(image, releaser);
}

}  // namespace compositor
