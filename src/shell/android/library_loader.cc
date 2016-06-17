// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/base_jni_onload.h"
#include "base/android/base_jni_registrar.h"
#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "base/bind.h"
#include "mojo/android/system/base_run_loop.h"
#include "mojo/android/system/core_impl.h"
#include "services/native_viewport/platform_viewport_android.h"
#include "shell/android/android_handler.h"
#include "shell/android/java_application_loader.h"
#include "shell/android/main.h"
#include "shell/android/native_handler_thread.h"
#include "ui/gl/android/gl_jni_registrar.h"

namespace {

base::android::RegistrationMethod kMojoRegisteredMethods[] = {
    {"Base", base::android::RegisterJni},
    {"Core", mojo::android::RegisterCoreImpl},
    {"BaseRunLoop", mojo::android::RegisterBaseRunLoop},
    {"AndroidHandler", shell::RegisterAndroidHandlerJni},
    {"JavaApplicationLoader", shell::JavaApplicationLoader::RegisterJni},
    {"NativeHandlerThread", shell::RegisterNativeHandlerThreadJni},
    {"PlatformViewportAndroid",
     native_viewport::PlatformViewportAndroid::Register},
    {"ShellService", shell::RegisterShellService},
};

bool RegisterJNI(JNIEnv* env) {
  return RegisterNativeMethods(env, kMojoRegisteredMethods,
                               arraysize(kMojoRegisteredMethods));
}

}  // namespace

// This is called by the VM when the shared library is first loaded.
JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  std::vector<base::android::RegisterCallback> register_callbacks;
  register_callbacks.push_back(base::Bind(&RegisterJNI));
  register_callbacks.push_back(base::Bind(&ui::gl::android::RegisterJni));
  if (!base::android::OnJNIOnLoadRegisterJNI(vm, register_callbacks) ||
      !base::android::OnJNIOnLoadInit(
          std::vector<base::android::InitCallback>())) {
    return -1;
  }

  return JNI_VERSION_1_4;
}
