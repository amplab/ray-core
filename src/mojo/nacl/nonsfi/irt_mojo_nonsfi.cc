// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/nacl/nonsfi/irt_mojo_nonsfi.h"

#include "mojo/public/c/system/buffer.h"
#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/time.h"
#include "mojo/public/c/system/wait.h"
#include "mojo/public/platform/nacl/mgl_irt.h"
#include "mojo/public/platform/nacl/mojo_irt.h"
#include "native_client/src/public/irt_core.h"
#include "native_client/src/untrusted/irt/irt_dev.h"

namespace {

MojoHandle g_mojo_handle = MOJO_HANDLE_INVALID;
bool g_running_translator = false;

const struct nacl_irt_mojo kIrtMojo = {
    nacl::MojoGetInitialHandle,
    MojoGetTimeTicksNow,
    MojoClose,
    MojoGetRights,
    MojoReplaceHandleWithReducedRights,
    MojoDuplicateHandleWithReducedRights,
    MojoDuplicateHandle,
    MojoWait,
    MojoWaitMany,
    MojoCreateMessagePipe,
    MojoWriteMessage,
    MojoReadMessage,
    MojoCreateDataPipe,
    MojoSetDataPipeProducerOptions,
    MojoGetDataPipeProducerOptions,
    MojoWriteData,
    MojoBeginWriteData,
    MojoEndWriteData,
    MojoSetDataPipeConsumerOptions,
    MojoGetDataPipeConsumerOptions,
    MojoReadData,
    MojoBeginReadData,
    MojoEndReadData,
    MojoCreateSharedBuffer,
    MojoDuplicateBufferHandle,
    MojoGetBufferInformation,
    MojoMapBuffer,
    MojoUnmapBuffer,
};

const struct nacl_irt_mgl kIrtMGL = {
    MGLCreateContext,
    MGLDestroyContext,
    MGLMakeCurrent,
    MGLGetCurrentContext,
    MGLGetProcAddress,
};

const struct nacl_irt_mgl_onscreen kIrtMGLOnScreen = {
    MGLResizeSurface,
    MGLSwapBuffers,
};

const struct nacl_irt_mgl_signal_sync_point kIrtMGLSignalSyncPoint = {
    MGLSignalSyncPoint,
};

int NotPNaClFilter() {
  return g_running_translator;
}

const struct nacl_irt_interface kIrtInterfaces[] = {
    // Interface to call Mojo functions
    { NACL_IRT_MOJO_v0_1,
      &kIrtMojo,
      sizeof(kIrtMojo),
      nullptr },
    // Interface to call PNaCl translation
    { NACL_IRT_PRIVATE_PNACL_TRANSLATOR_COMPILE_v0_1,
      &nacl::nacl_irt_private_pnacl_translator_compile,
      sizeof(nacl_irt_private_pnacl_translator_compile),
      NotPNaClFilter },
    // Interface to call PNaCl linking
    { NACL_IRT_PRIVATE_PNACL_TRANSLATOR_LINK_v0_1,
      &nacl::nacl_irt_private_pnacl_translator_link,
      sizeof(nacl_irt_private_pnacl_translator_link),
      NotPNaClFilter },
    // Adds mechanism for opening object files, like crtbegin.o
    { NACL_IRT_RESOURCE_OPEN_v0_1,
      &nacl::nacl_irt_resource_open,
      sizeof(nacl_irt_resource_open),
      NotPNaClFilter },
    // GPU functions which give control over MGL context
    { NACL_IRT_MGL_v0_1,
      &kIrtMGL,
      sizeof(kIrtMGL),
      nullptr },
    // GPU functions which update framebuffer with respect to the display
    { NACL_IRT_MGL_ONSCREEN_v0_1,
      &kIrtMGLOnScreen,
      sizeof(kIrtMGLOnScreen),
      nullptr },
    // GPU functions to synchronize CPU and GPU services
    { NACL_IRT_MGL_SIGNAL_SYNC_POINT_v0_1,
      &kIrtMGLSignalSyncPoint,
      sizeof(kIrtMGLSignalSyncPoint),
      nullptr },
};

}  // namespace

namespace nacl {

MojoResult MojoGetInitialHandle(MojoHandle* handle) {
  *handle = g_mojo_handle;
  return MOJO_RESULT_OK;
}

void MojoSetInitialHandle(MojoHandle handle) {
  g_mojo_handle = handle;
}

void MojoPnaclTranslatorEnable() {
  g_running_translator = true;
}

size_t MojoIrtNonsfiQuery(const char* interface_ident,
                          void* table,
                          size_t tablesize) {
  size_t result = nacl_irt_query_list(interface_ident, table, tablesize,
                                      kIrtInterfaces, sizeof(kIrtInterfaces));
  if (result != 0)
    return result;
  return nacl_irt_query_core(interface_ident, table, tablesize);
}

}  // namespace nacl
