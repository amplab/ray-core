// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is an example of an Android Mojo app that is composed of native code and
// Java code that the native code calls into. In particular, we call to Java
// upon entering MojoMain to retrieve the name of the device that the
// application is running on and print it out to the log.
//
// To run the example:
//  - build and install the MojoShell on the device:
//        ninja -C some_place mojo_shell_apk
//        adb install -r some_place/apks/MojoShell.apk
//  - install forwarder on the device:
//        ninja -C some_place forwarder
//        adb push some_place/forwarder /data/tmp/forwarder
//  - run the forwarder on the device:
//        adb shell /data/tmp/forwarder 4444:4444
//  - build the example:
//        ninja -C some_place examples/device_name
//  - set up the http server in your output directory:
//        python -m SimpleHTTPServer 4444
//  - build/android/adb_run_mojo_shell
//        http://127.0.0.1:4444/obj/examples/device_name/device_name.mojo

#include "base/android/jni_string.h"
#include "jni/DeviceName_jni.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/c/system/time.h"

namespace mojo {

namespace examples {

bool RegisterDeviceJni(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

// Simplest example of a function that needs to call into Java.
std::string GetDeviceName() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return base::android::ConvertJavaStringToUTF8(
      env, Java_DeviceName_getName(env).obj());
}

// Example of a function that need to access the application context - see
// InitApplicationContext() below.
std::string GetApplicationClassName() {
  JNIEnv* env = base::android::AttachCurrentThread();
  jobject context = base::android::GetApplicationContext();
  return base::android::ConvertJavaStringToUTF8(
      env, Java_DeviceName_getApplicationClassName(env, context).obj());
}

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  // Call a Java function to demonstrate that the JNI was correctly set up.
  LOG(INFO) << "Device name: " << mojo::examples::GetDeviceName();

  // Call a function that uses the application context to demonstrate that it
  // was properly set.
  LOG(INFO) << "Application class: "
            << mojo::examples::GetApplicationClassName();

  // Call a Mojo core function to demonstrate that the thunks were properly set.
  MojoTimeTicks ticks = MojoGetTimeTicksNow();
  LOG(INFO) << "Timeticks: " << ticks;

  return MOJO_RESULT_OK;
}

JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  base::android::InitVM(vm);
  JNIEnv* env = base::android::AttachCurrentThread();

  if (!mojo::examples::RegisterDeviceJni(env))
    return -1;

  return JNI_VERSION_1_4;
}

// This is needed only if the application actually needs to access the
// application context. For instance, GetDeviceName() itself doesn't need it,
// but we use it for demonstrative purposes in GetApplicationClassName().
extern "C" JNI_EXPORT void InitApplicationContext(
    const base::android::JavaRef<jobject>& context) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::InitApplicationContext(env, context);
}
