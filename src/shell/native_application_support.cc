// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/native_application_support.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
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
