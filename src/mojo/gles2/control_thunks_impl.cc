// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/gles2/control_thunks_impl.h"

#include <memory>

#include "mojo/gles2/gles2_context.h"
#include "mojo/public/cpp/system/message_pipe.h"

extern "C" {

#define VISIT_GL_CALL(Function, ReturnType, PARAMETERS, ARGUMENTS) \
  ReturnType MojoGLES2gl##Function PARAMETERS;
#include "mojo/public/platform/native/gles2/call_visitor.h"
#undef VISIT_GL_CALL

}

namespace gles2 {

// static
ControlThunksImpl* ControlThunksImpl::Get() {
  static auto* thunks = new ControlThunksImpl;
  return thunks;
}

MGLContext ControlThunksImpl::CreateContext(
    MGLOpenGLAPIVersion version,
    MojoHandle command_buffer_handle,
    MGLContext share_group,
    MGLContextLostCallback lost_callback,
    void* lost_callback_closure,
    const struct MojoAsyncWaiter* async_waiter) {
  mojo::MessagePipeHandle mph(command_buffer_handle);
  mojo::ScopedMessagePipeHandle scoped_handle(mph);
  std::unique_ptr<GLES2Context> client(
      new GLES2Context(async_waiter, scoped_handle.Pass(), lost_callback,
                       lost_callback_closure));
  if (!client->Initialize())
    client.reset();
  return client.release();
}

void ControlThunksImpl::DestroyContext(MGLContext context) {
  delete static_cast<GLES2Context*>(context);
}

void ControlThunksImpl::MakeCurrent(MGLContext context) {
  current_context_tls_.Set(static_cast<GLES2Context*>(context));
}

MGLContext ControlThunksImpl::GetCurrentContext() {
  return current_context_tls_.Get();
}

void ControlThunksImpl::ResizeSurface(uint32_t width, uint32_t height) {
  current_context_tls_.Get()->interface()->ResizeCHROMIUM(width, height, 1.f);
}

void ControlThunksImpl::SwapBuffers() {
  current_context_tls_.Get()->interface()->SwapBuffers();
}


MGLMustCastToProperFunctionPointerType ControlThunksImpl::GetProcAddress(
    const char* name) {
#define VISIT_GL_CALL(Function, ReturnType, PARAMETERS, ARGUMENTS) \
  if (!strcmp(name, "gl"#Function)) \
    return reinterpret_cast<MGLMustCastToProperFunctionPointerType>( \
        MojoGLES2gl##Function);
#include "mojo/public/platform/native/gles2/call_visitor.h"
#undef VISIT_GL_CALL
  return nullptr;
}

void* ControlThunksImpl::GetGLES2Interface(MGLContext context) {
  GLES2Context* client = reinterpret_cast<GLES2Context*>(context);
  DCHECK(client);
  return client->interface();
}

void ControlThunksImpl::Echo(MGLEchoCallback callback, void* closure) {
  current_context_tls_.Get()->Echo(callback, closure);
}

void ControlThunksImpl::SignalSyncPoint(
    uint32_t sync_point,
    MGLSignalSyncPointCallback callback,
    void* closure) {
  current_context_tls_.Get()->context_support()->SignalSyncPoint(
      sync_point, base::Bind(callback, closure));
}

gpu::gles2::GLES2Interface* ControlThunksImpl::CurrentGLES2Interface() {
  if (!current_context_tls_.Get())
    return nullptr;
  return current_context_tls_.Get()->interface();
}

ControlThunksImpl::ControlThunksImpl() {
}

ControlThunksImpl::~ControlThunksImpl() {
}

}  // namespace gles2
