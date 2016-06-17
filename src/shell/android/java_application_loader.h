// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_JAVA_APPLICATION_LOADER_H_
#define SHELL_ANDROID_JAVA_APPLICATION_LOADER_H_

#include "base/android/scoped_java_ref.h"
#include "shell/application_manager/application_loader.h"

namespace shell {
class JavaApplicationLoader : public ApplicationLoader {
 public:
  JavaApplicationLoader(const base::android::ScopedJavaGlobalRef<jobject>&
                            java_application_registry,
                        const std::string& application_url);

  static base::android::ScopedJavaLocalRef<jobject>
  CreateJavaApplicationRegistry();

  static std::vector<std::string> GetApplicationURLs(
      jobject java_application_registry);

  static bool RegisterJni(JNIEnv* env);

 private:
  // ApplicationLoader overrides:
  void Load(
      const GURL& url,
      mojo::InterfaceRequest<mojo::Application> application_request) override;

  base::android::ScopedJavaGlobalRef<jobject> java_application_registry_;
  base::android::ScopedJavaGlobalRef<jstring> application_url_;
};
}  // namespace shell

#endif  // SHELL_ANDROID_JAVA_APPLICATION_LOADER_H_
