// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/native_application_support.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "mojo/public/platform/native/gles2_impl_chromium_bind_uniform_location_thunks.h"
#include "mojo/public/platform/native/gles2_impl_chromium_map_sub_thunks.h"
#include "mojo/public/platform/native/gles2_impl_chromium_miscellaneous_thunks.h"
#include "mojo/public/platform/native/gles2_impl_chromium_resize_thunks.h"
#include "mojo/public/platform/native/gles2_impl_chromium_sync_point_thunks.h"
#include "mojo/public/platform/native/gles2_impl_chromium_texture_mailbox_thunks.h"
#include "mojo/public/platform/native/gles2_impl_ext_debug_marker_thunks.h"
#include "mojo/public/platform/native/gles2_impl_ext_discard_framebuffer_thunks.h"
#include "mojo/public/platform/native/gles2_impl_ext_multisampled_render_to_texture_thunks.h"
#include "mojo/public/platform/native/gles2_impl_ext_occlusion_query_thunks.h"
#include "mojo/public/platform/native/gles2_impl_ext_texture_storage_thunks.h"
#include "mojo/public/platform/native/gles2_impl_khr_blend_equation_advanced_thunks.h"
#include "mojo/public/platform/native/gles2_impl_oes_vertex_array_object_thunks.h"
#include "mojo/public/platform/native/gles2_impl_thunks.h"
#include "mojo/public/platform/native/mgl_echo_thunks.h"
#include "mojo/public/platform/native/mgl_onscreen_thunks.h"
#include "mojo/public/platform/native/mgl_signal_sync_point_thunks.h"
#include "mojo/public/platform/native/mgl_thunks.h"
#include "mojo/public/platform/native/platform_handle_private_thunks.h"
#include "mojo/public/platform/native/system_thunks.h"

namespace shell {

namespace {

template <typename Thunks>
bool SetThunks(Thunks (*make_thunks)(),
               const char* function_name,
               base::NativeLibrary library) {
  typedef size_t (*SetThunksFn)(const Thunks* thunks);
  SetThunksFn set_thunks = reinterpret_cast<SetThunksFn>(
      base::GetFunctionPointerFromNativeLibrary(library, function_name));
  if (!set_thunks)
    return false;
  Thunks thunks = make_thunks();
  size_t expected_size = set_thunks(&thunks);
  if (expected_size > sizeof(Thunks)) {
    LOG(ERROR) << "Invalid app library: expected " << function_name
               << " to return thunks of size: " << expected_size;
    return false;
  }
  return true;
}

}  // namespace

base::NativeLibrary LoadNativeApplication(const base::FilePath& app_path) {
  DVLOG(2) << "Loading Mojo app in process from library: " << app_path.value();

  base::NativeLibraryLoadError error;
  base::NativeLibrary app_library = base::LoadNativeLibrary(app_path, &error);
  LOG_IF(ERROR, !app_library)
      << "Failed to load app library (error: " << error.ToString() << ")";
  return app_library;
}

bool RunNativeApplication(
    base::NativeLibrary app_library,
    mojo::InterfaceRequest<mojo::Application> application_request) {
  // Tolerate |app_library| being null, to make life easier for callers.
  if (!app_library)
    return false;

  if (!SetThunks(&MojoMakeSystemThunks, "MojoSetSystemThunks", app_library)) {
    LOG(ERROR) << "MojoSetSystemThunks not found";
    return false;
  }

  // TODO(freiling): enforce the private nature of this API, somehow?
  SetThunks(&MojoMakePlatformHandlePrivateThunks,
            "MojoSetPlatformHandlePrivateThunks", app_library);

  SetThunks(&MojoMakeGLES2ImplThunks, "MojoSetGLES2ImplThunks",
            app_library);
  SetThunks(MojoMakeGLES2ImplEXTDebugMarkerThunks,
            "MojoSetGLES2ImplEXTDebugMarkerThunks", app_library);
  SetThunks(MojoMakeGLES2ImplEXTDiscardFramebufferThunks,
            "MojoSetGLES2ImplEXTDiscardFramebufferThunks", app_library);
  SetThunks(MojoMakeGLES2ImplEXTOcclusionQueryThunks,
            "MojoSetGLES2ImplEXTOcclusionQueryThunks", app_library);
  SetThunks(MojoMakeGLES2ImplEXTTextureStorageThunks,
            "MojoSetGLES2ImplEXTTextureStorageThunks", app_library);
  SetThunks(MojoMakeGLES2ImplEXTMultisampledRenderToTextureThunks,
            "MojoSetGLES2ImplEXTMultisampledRenderToTextureThunks",
            app_library);
  SetThunks(MojoMakeGLES2ImplKHRBlendEquationAdvancedThunks,
            "MojoSetGLES2ImplKHRBlendEquationAdvancedThunks", app_library);
  SetThunks(MojoMakeGLES2ImplOESVertexArrayObjectThunks,
            "MojoSetGLES2ImplOESVertexArrayObjectThunks", app_library);
  // Deprecated name for "MojoSetGLES2ImplEXTOcclusionQueryThunks" (TODO(vtl):
  // when no app is using this name any longer, delete it):
  SetThunks(MojoMakeGLES2ImplEXTOcclusionQueryThunks,
            "MojoSetGLES2ImplOcclusionQueryEXTThunks", app_library);
  // "Chromium" extensions:
  SetThunks(MojoMakeGLES2ImplCHROMIUMBindUniformLocationThunks,
            "MojoSetGLES2ImplCHROMIUMBindUniformLocationThunks", app_library);
  SetThunks(MojoMakeGLES2ImplCHROMIUMMapSubThunks,
            "MojoSetGLES2ImplCHROMIUMMapSubThunks", app_library);
  SetThunks(MojoMakeGLES2ImplCHROMIUMMiscellaneousThunks,
            "MojoSetGLES2ImplCHROMIUMMiscellaneousThunks", app_library);
  SetThunks(MojoMakeGLES2ImplCHROMIUMResizeThunks,
            "MojoSetGLES2ImplCHROMIUMResizeThunks", app_library);
  SetThunks(MojoMakeGLES2ImplCHROMIUMSyncPointThunks,
            "MojoSetGLES2ImplCHROMIUMSyncPointThunks", app_library);
  SetThunks(MojoMakeGLES2ImplCHROMIUMTextureMailboxThunks,
            "MojoSetGLES2ImplCHROMIUMTextureMailboxThunks", app_library);

  if (SetThunks(MojoMakeMGLThunks, "MojoSetMGLThunks", app_library)) {
    SetThunks(MojoMakeMGLEchoThunks, "MojoSetMGLEchoThunks", app_library);

    // TODO(jamesr): We should only need to expose the onscreen thunks to apps
    // that need to draw to the screen like the system compositor.
    SetThunks(MojoMakeMGLOnscreenThunks, "MojoSetMGLOnscreenThunks",
              app_library);

    SetThunks(MojoMakeMGLSignalSyncPointThunks,
              "MojoSetMGLSignalSyncPointThunks", app_library);
  }

  typedef MojoResult (*MojoMainFunction)(MojoHandle);
  MojoMainFunction main_function = reinterpret_cast<MojoMainFunction>(
      base::GetFunctionPointerFromNativeLibrary(app_library, "MojoMain"));
  if (!main_function) {
    LOG(ERROR) << "MojoMain not found";
    return false;
  }
  // |MojoMain()| takes ownership of the Application request handle.
  MojoHandle handle = application_request.PassMessagePipe().release().value();
  MojoResult result = main_function(handle);
  LOG_IF(ERROR, result != MOJO_RESULT_OK)
      << "MojoMain returned error (result: " << result << ")";
  return true;
}

}  // namespace shell
