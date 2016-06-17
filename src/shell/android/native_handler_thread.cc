// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/native_handler_thread.h"

#include "base/message_loop/message_loop.h"
#include "jni/NativeHandlerThread_jni.h"

namespace shell {

jlong CreateMessageLoop(JNIEnv* env, const JavaParamRef<jobject>& jcaller) {
  scoped_ptr<base::MessageLoop> loop(new base::MessageLoopForUI);
  base::MessageLoopForUI::current()->Start();
  return reinterpret_cast<intptr_t>(loop.release());
}

void DeleteMessageLoop(JNIEnv* env,
                       const JavaParamRef<jobject>& jcaller,
                       jlong message_loop) {
  scoped_ptr<base::MessageLoop> loop(
      reinterpret_cast<base::MessageLoop*>(message_loop));
  loop->QuitNow();
}

bool RegisterNativeHandlerThreadJni(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace shell
