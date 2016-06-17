// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/android_handler.h"

#include <fcntl.h>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/rand_util.h"
#include "base/run_loop.h"
#include "base/scoped_native_library.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "jni/AndroidHandler_jni.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "shell/android/run_android_application_function.h"
#include "shell/native_application_support.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetApplicationContext;

namespace shell {

namespace {

// This function loads the application library, sets the application context and
// thunks and calls into the application MojoMain. To ensure that the thunks are
// set correctly we keep it in the Mojo shell .so and pass the function pointer
// to the helper libbootstrap.so.
void RunAndroidApplication(JNIEnv* env,
                           jobject j_context,
                           uintptr_t tracing_id,
                           const base::FilePath& app_path,
                           jint j_handle) {
  mojo::InterfaceRequest<mojo::Application> application_request =
      mojo::InterfaceRequest<mojo::Application>(
          mojo::MakeScopedHandle(mojo::MessagePipeHandle(j_handle)));

  // Load the library, so that we can set the application context there if
  // needed.
  // TODO(vtl): We'd use a ScopedNativeLibrary, but it doesn't have .get()!
  base::NativeLibrary app_library = LoadNativeApplication(app_path);
  if (!app_library)
    return;

  // Set the application context if needed. Most applications will need to
  // access the Android ApplicationContext in which they are run. If the
  // application library exports the InitApplicationContext function, we will
  // set it there.
  const char* init_application_context_name = "InitApplicationContext";
  typedef void (*InitApplicationContextFn)(
      const base::android::JavaRef<jobject>&);
  InitApplicationContextFn init_application_context =
      reinterpret_cast<InitApplicationContextFn>(
          base::GetFunctionPointerFromNativeLibrary(
              app_library, init_application_context_name));
  if (init_application_context) {
    base::android::ScopedJavaLocalRef<jobject> scoped_context(env, j_context);
    init_application_context(scoped_context);
  }

  // The application is ready to be run.
  TRACE_EVENT_ASYNC_END0("android_handler", "AndroidHandler::RunApplication",
                         tracing_id);

  // Run the application.
  RunNativeApplication(app_library, application_request.Pass());
  // TODO(vtl): See note about unloading and thread-local destructors above
  // declaration of |LoadNativeApplication()|.
  base::UnloadNativeLibrary(app_library);
}

}  // namespace

AndroidHandler::AndroidHandler() {}

AndroidHandler::~AndroidHandler() {}

void AndroidHandler::RunApplication(
    mojo::InterfaceRequest<mojo::Application> application_request,
    mojo::URLResponsePtr response) {
  JNIEnv* env = AttachCurrentThread();
  uintptr_t tracing_id = reinterpret_cast<uintptr_t>(this);
  TRACE_EVENT_ASYNC_BEGIN1("android_handler", "AndroidHandler::RunApplication",
                           tracing_id, "url", std::string(response->url));
  base::FilePath extracted_dir;
  base::FilePath cache_dir;
  {
    base::MessageLoop loop;
    handler_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&AndroidHandler::ExtractApplication, base::Unretained(this),
                   base::Unretained(&extracted_dir),
                   base::Unretained(&cache_dir), base::Passed(response.Pass()),
                   base::Bind(base::IgnoreResult(
                                  &base::SingleThreadTaskRunner::PostTask),
                              loop.task_runner(), FROM_HERE,
                              base::MessageLoop::QuitWhenIdleClosure())));
    base::RunLoop().Run();
  }

  ScopedJavaLocalRef<jstring> j_extracted_dir =
      ConvertUTF8ToJavaString(env, extracted_dir.value());
  ScopedJavaLocalRef<jstring> j_cache_dir =
      ConvertUTF8ToJavaString(env, cache_dir.value());
  RunAndroidApplicationFn run_android_application_fn = &RunAndroidApplication;
  Java_AndroidHandler_bootstrap(
      env, GetApplicationContext(), tracing_id, j_extracted_dir.obj(),
      j_cache_dir.obj(),
      application_request.PassMessagePipe().release().value(),
      reinterpret_cast<jlong>(run_android_application_fn));
}

void AndroidHandler::OnInitialize() {
  handler_task_runner_ = base::MessageLoop::current()->task_runner();
  mojo::ConnectToService(shell(), "mojo:url_response_disk_cache",
                         GetProxy(&url_response_disk_cache_));
}

bool AndroidHandler::OnAcceptConnection(
    mojo::ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<mojo::ContentHandler>(
      mojo::ContentHandlerFactory::GetInterfaceRequestHandler(this));
  return true;
}

void AndroidHandler::ExtractApplication(base::FilePath* extracted_dir,
                                        base::FilePath* cache_dir,
                                        mojo::URLResponsePtr response,
                                        const base::Closure& callback) {
  url_response_disk_cache_->UpdateAndGetExtracted(
      response.Pass(),
      [extracted_dir, cache_dir, callback](mojo::Array<uint8_t> extracted_path,
                                           mojo::Array<uint8_t> cache_path) {
        if (extracted_path.is_null()) {
          *extracted_dir = base::FilePath();
          *cache_dir = base::FilePath();
        } else {
          *extracted_dir = base::FilePath(
              std::string(reinterpret_cast<char*>(&extracted_path.front()),
                          extracted_path.size()));
          *cache_dir = base::FilePath(std::string(
              reinterpret_cast<char*>(&cache_path.front()), cache_path.size()));
        }
        callback.Run();
      });
}

ScopedJavaLocalRef<jstring> CreateTemporaryFile(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jstring>& j_directory,
    const JavaParamRef<jstring>& j_basename,
    const JavaParamRef<jstring>& j_extension) {
  std::string basename(ConvertJavaStringToUTF8(env, j_basename));
  std::string extension(ConvertJavaStringToUTF8(env, j_extension));
  base::FilePath directory(ConvertJavaStringToUTF8(env, j_directory));

  for (;;) {
    std::string random = base::RandBytesAsString(16);
    std::string filename =
        basename + base::HexEncode(random.data(), random.size()) + extension;
    base::FilePath temporary_file = directory.Append(filename);
    int fd = open(temporary_file.value().c_str(), O_CREAT | O_EXCL, 0600);
    if (fd != -1) {
      close(fd);
      return ConvertUTF8ToJavaString(env, temporary_file.value());
    }
  }
}

bool RegisterAndroidHandlerJni(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace shell
