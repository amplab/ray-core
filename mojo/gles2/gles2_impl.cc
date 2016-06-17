// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/gles2_interface.h"
#include "mojo/gles2/control_thunks_impl.h"
#include "mojo/public/c/gpu/GLES2/gl2.h"
#include "mojo/public/c/gpu/MGL/mgl.h"

extern "C" {

#define VISIT_GL_CALL(Function, ReturnType, PARAMETERS, ARGUMENTS)             \
  ReturnType GL_APIENTRY gl##Function PARAMETERS {                             \
    auto interface = gles2::ControlThunksImpl::Get()->CurrentGLES2Interface(); \
    DCHECK(interface);                                                         \
    return interface->Function ARGUMENTS;                                      \
  }
#include "mojo/public/platform/native/gles2/call_visitor_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_chromium_bind_uniform_location_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_chromium_map_sub_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_chromium_miscellaneous_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_chromium_resize_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_chromium_sync_point_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_chromium_texture_mailbox_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_ext_debug_marker_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_ext_discard_framebuffer_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_ext_multisampled_render_to_texture_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_ext_occlusion_query_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_ext_texture_storage_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_khr_blend_equation_advanced_autogen.h"
#include "mojo/public/platform/native/gles2/call_visitor_oes_vertex_array_object_autogen.h"
#undef VISIT_GL_CALL

}  // extern "C"
