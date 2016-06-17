// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_NATIVE_HANDLER_THREAD_H_
#define SHELL_ANDROID_NATIVE_HANDLER_THREAD_H_

#include <jni.h>

namespace shell {

bool RegisterNativeHandlerThreadJni(JNIEnv* env);

}  // namespace shell

#endif  // SHELL_ANDROID_NATIVE_HANDLER_THREAD_H_
