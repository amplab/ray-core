// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_ANDROID_ANDROID_VSYNC_PROVIDER_H_
#define UI_GL_ANDROID_ANDROID_VSYNC_PROVIDER_H_

#include "ui/gfx/vsync_provider.h"

#include "base/android/scoped_java_ref.h"
#include "ui/gfx/vsync_provider.h"

namespace gfx {

class AndroidVSyncProvider : public VSyncProvider {
 public:
  AndroidVSyncProvider();
  ~AndroidVSyncProvider() override;

  void OnSyncChanged(JNIEnv* env, jobject caller, jlong time, jlong delay);

  static bool RegisterVSyncProvider(JNIEnv* env);

 private:
  void GetVSyncParameters(const UpdateVSyncCallback& callback) override;

  base::android::ScopedJavaGlobalRef<jobject> j_vsync_provider_;
  UpdateVSyncCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(AndroidVSyncProvider);
};

}  // namespace gfx

#endif  // UI_GL_ANDROID_ANDROID_VSYNC_PROVIDER_H_
