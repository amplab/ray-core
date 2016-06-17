// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/java_application_loader.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "jni/JavaApplicationRegistry_jni.h"

namespace shell {

JavaApplicationLoader::JavaApplicationLoader(
    const base::android::ScopedJavaGlobalRef<jobject>&
        java_application_registry,
    const std::string& application_url)
    : java_application_registry_(java_application_registry) {
  JNIEnv* env = base::android::AttachCurrentThread();
  application_url_.Reset(
      env, base::android::ConvertUTF8ToJavaString(env, application_url).obj());
}

// static
base::android::ScopedJavaLocalRef<jobject>
JavaApplicationLoader::CreateJavaApplicationRegistry() {
  return Java_JavaApplicationRegistry_create(
      base::android::AttachCurrentThread());
}

// static
std::vector<std::string> JavaApplicationLoader::GetApplicationURLs(
    jobject java_application_registry) {
  std::vector<std::string> result;
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobjectArray> urls =
      Java_JavaApplicationRegistry_getApplications(env,
                                                   java_application_registry);

  base::android::AppendJavaStringArrayToStringVector(env, urls.obj(), &result);
  return result;
}

// static
bool JavaApplicationLoader::RegisterJni(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void JavaApplicationLoader::Load(
    const GURL& url,
    mojo::InterfaceRequest<mojo::Application> application_request) {
  Java_JavaApplicationRegistry_startApplication(
      base::android::AttachCurrentThread(), java_application_registry_.obj(),
      application_url_.obj(),
      application_request.PassMessagePipe().release().value());
}

}  // namespace shell
