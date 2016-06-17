// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/android/android_vsync_provider.h"

#include "base/android/jni_android.h"
#include "base/time/time.h"
#include "jni/VSyncProvider_jni.h"

namespace gfx {

AndroidVSyncProvider::AndroidVSyncProvider() {
  j_vsync_provider_.Reset(
      Java_VSyncProvider_create(base::android::AttachCurrentThread()));
}

AndroidVSyncProvider::~AndroidVSyncProvider() {
  Java_VSyncProvider_stopMonitoring(base::android::AttachCurrentThread(),
                                    j_vsync_provider_.obj());
}

void AndroidVSyncProvider::OnSyncChanged(JNIEnv* env,
                                         jobject caller,
                                         jlong time,
                                         jlong delay) {
  callback_.Run(base::TimeTicks::FromInternalValue(time),
                base::TimeDelta::FromMicroseconds(delay));
}

// static
bool AndroidVSyncProvider::RegisterVSyncProvider(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void AndroidVSyncProvider::GetVSyncParameters(
    const UpdateVSyncCallback& callback) {
  callback_ = callback;
  Java_VSyncProvider_startMonitoring(base::android::AttachCurrentThread(),
                                     j_vsync_provider_.obj(),
                                     reinterpret_cast<intptr_t>(this));
}

}  // namespace gfx
