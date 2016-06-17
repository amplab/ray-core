// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated from
// gpu/command_buffer/build_gles2_cmd_buffer.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

VISIT_GL_CALL(RenderbufferStorageMultisampleEXT,
              void,
              (GLenum target,
               GLsizei samples,
               GLenum internalformat,
               GLsizei width,
               GLsizei height),
              (target, samples, internalformat, width, height))
VISIT_GL_CALL(FramebufferTexture2DMultisampleEXT,
              void,
              (GLenum target,
               GLenum attachment,
               GLenum textarget,
               GLuint texture,
               GLint level,
               GLsizei samples),
              (target, attachment, textarget, texture, level, samples))
