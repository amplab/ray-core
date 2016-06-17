// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated from
// gpu/command_buffer/build_gles2_cmd_buffer.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

#include "mojo/public/platform/native/gles2_impl_chromium_miscellaneous_thunks.h"

#include <assert.h>

#include "mojo/public/platform/native/thunk_export.h"

static struct MojoGLES2ImplCHROMIUMMiscellaneousThunks
    g_impl_chromium_miscellaneous_thunks = {0};

#define VISIT_GL_CALL(Function, ReturnType, PARAMETERS, ARGUMENTS)  \
  ReturnType GL_APIENTRY gl##Function PARAMETERS {                  \
    assert(g_impl_chromium_miscellaneous_thunks.Function);          \
    return g_impl_chromium_miscellaneous_thunks.Function ARGUMENTS; \
  }
#include "mojo/public/platform/native/gles2/call_visitor_chromium_miscellaneous_autogen.h"
#undef VISIT_GL_CALL

THUNK_EXPORT size_t MojoSetGLES2ImplCHROMIUMMiscellaneousThunks(
    const struct MojoGLES2ImplCHROMIUMMiscellaneousThunks*
        gles2_impl_chromium_miscellaneous_thunks) {
  if (gles2_impl_chromium_miscellaneous_thunks->size >=
      sizeof(g_impl_chromium_miscellaneous_thunks))
    g_impl_chromium_miscellaneous_thunks =
        *gles2_impl_chromium_miscellaneous_thunks;
  return sizeof(g_impl_chromium_miscellaneous_thunks);
}
