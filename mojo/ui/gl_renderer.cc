// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/ui/gl_renderer.h"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <GLES2/gl2.h>
#include <GLES2/gl2extmojo.h>

namespace mojo {
namespace ui {

GLRenderer::GLRenderer(const scoped_refptr<mojo::GLContext>& gl_context,
                       uint32_t max_recycled_textures)
    : gl_context_(gl_context),
      max_recycled_textures_(max_recycled_textures),
      weak_factory_(this) {
  DCHECK(gl_context_);
}

GLRenderer::~GLRenderer() {}

std::unique_ptr<mojo::GLTexture> GLRenderer::GetTexture(
    const mojo::GLContext::Scope& gl_scope,
    const mojo::Size& requested_size) {
  DCHECK(gl_scope.gl_context() == gl_context_);

  while (!recycled_textures_.empty()) {
    GLRecycledTextureInfo texture_info(std::move(recycled_textures_.front()));
    recycled_textures_.pop_front();
    if (texture_info.first->size().Equals(requested_size)) {
      glWaitSyncPointCHROMIUM(texture_info.second);
      return std::move(texture_info.first);
    }
  }

  return std::unique_ptr<GLTexture>(new GLTexture(gl_scope, requested_size));
}

mojo::gfx::composition::ResourcePtr GLRenderer::BindTextureResource(
    const mojo::GLContext::Scope& gl_scope,
    std::unique_ptr<GLTexture> gl_texture,
    mojo::gfx::composition::MailboxTextureResource::Origin origin) {
  DCHECK(gl_scope.gl_context() == gl_context_);
  DCHECK(gl_texture->gl_context() == gl_context_);

  // Produce the texture.
  glBindTexture(GL_TEXTURE_2D, gl_texture->texture_id());
  GLbyte mailbox[GL_MAILBOX_SIZE_CHROMIUM];
  glGenMailboxCHROMIUM(mailbox);
  glProduceTextureCHROMIUM(GL_TEXTURE_2D, mailbox);
  glBindTexture(GL_TEXTURE_2D, 0);
  GLuint sync_point = glInsertSyncPointCHROMIUM();

  // Populate the resource description.
  auto resource = mojo::gfx::composition::Resource::New();
  resource->set_mailbox_texture(
      mojo::gfx::composition::MailboxTextureResource::New());
  resource->get_mailbox_texture()->mailbox_name.resize(sizeof(mailbox));
  memcpy(resource->get_mailbox_texture()->mailbox_name.data(), mailbox,
         sizeof(mailbox));
  resource->get_mailbox_texture()->sync_point = sync_point;
  resource->get_mailbox_texture()->size = gl_texture->size().Clone();
  resource->get_mailbox_texture()->origin = origin;
  resource->get_mailbox_texture()->callback =
      (new GLTextureReleaser(
           weak_factory_.GetWeakPtr(),
           GLRecycledTextureInfo(std::move(gl_texture), sync_point)))
          ->StrongBind()
          .Pass();

  bound_textures_++;
  DVLOG(2) << "bind: bound_textures=" << bound_textures_;
  return resource;
}

mojo::gfx::composition::ResourcePtr GLRenderer::DrawGL(
    const mojo::GLContext::Scope& gl_scope,
    const mojo::Size& size,
    bool with_depth,
    const DrawGLCallback& callback) {
  DCHECK(gl_scope.gl_context() == gl_context_);

  std::unique_ptr<mojo::GLTexture> texture = GetTexture(gl_scope, size);
  DCHECK(texture);

  GLuint fbo = 0u;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture->texture_id(), 0);

  GLuint depth_buffer = 0u;
  if (with_depth) {
    glGenRenderbuffers(1, &depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, size.width,
                          size.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth_buffer);
  }

#if DCHECK_IS_ON()
  // This check causes a flush of the GL pipeline which is too expensive
  // even in debug mode so only check it the first time as a sanity check.
  if (!checked_framebuffer_status_) {
    DCHECK_EQ(static_cast<GLenum>(GL_FRAMEBUFFER_COMPLETE),
              glCheckFramebufferStatus(GL_FRAMEBUFFER));
    checked_framebuffer_status_ = true;
  }
#endif

  glViewport(0, 0, size.width, size.height);
  callback.Run(gl_scope, size);

  if (with_depth)
    glDeleteRenderbuffers(1, &depth_buffer);
  glDeleteFramebuffers(1, &fbo);

  return BindTextureResource(gl_scope, std::move(texture));
}

mojo::gfx::composition::ResourcePtr GLRenderer::DrawGL(
    const mojo::Size& size,
    bool with_depth,
    const DrawGLCallback& callback) {
  if (gl_context_->is_lost())
    return nullptr;
  mojo::GLContext::Scope gl_scope(gl_context_);
  return DrawGL(gl_scope, size, with_depth, callback);
}

void GLRenderer::ReleaseTexture(GLRecycledTextureInfo texture_info,
                                bool recyclable) {
  DCHECK(bound_textures_);
  bound_textures_--;
  if (recyclable && recycled_textures_.size() < max_recycled_textures_) {
    recycled_textures_.emplace_back(std::move(texture_info));
  }
  DVLOG(2) << "release: bound_textures=" << bound_textures_
           << ", recycled_textures=" << recycled_textures_.size();
}

GLRenderer::GLTextureReleaser::GLTextureReleaser(
    const base::WeakPtr<GLRenderer>& provider,
    GLRecycledTextureInfo info)
    : provider_(provider), texture_info_(std::move(info)), binding_(this) {}

GLRenderer::GLTextureReleaser::~GLTextureReleaser() {
  // It's possible for the object to be destroyed due to a connection
  // error on the callback pipe.  When this happens we don't want to
  // recycle the texture since we have too little knowledge about its
  // state to confirm that it will be safe to do so.
  Release(false /*recyclable*/);
}

mojo::gfx::composition::MailboxTextureCallbackPtr
GLRenderer::GLTextureReleaser::StrongBind() {
  mojo::gfx::composition::MailboxTextureCallbackPtr callback;
  binding_.Bind(mojo::GetProxy(&callback));
  return callback;
}

void GLRenderer::GLTextureReleaser::OnMailboxTextureReleased() {
  Release(true /*recyclable*/);
}

void GLRenderer::GLTextureReleaser::Release(bool recyclable) {
  if (provider_) {
    provider_->ReleaseTexture(std::move(texture_info_), recyclable);
    provider_.reset();
  }
}

}  // namespace ui
}  // namespace mojo
