// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_ANDROID_HANDLER_H_
#define SHELL_ANDROID_ANDROID_HANDLER_H_

#include <jni.h>

#include "base/single_thread_task_runner.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/services/content_handler/interfaces/content_handler.mojom.h"
#include "mojo/services/url_response_disk_cache/interfaces/url_response_disk_cache.mojom.h"

namespace base {
class FilePath;
}

namespace shell {

class AndroidHandler : public mojo::ApplicationImplBase,
                       public mojo::ContentHandlerFactory::Delegate {
 public:
  AndroidHandler();
  ~AndroidHandler();

 private:
  // mojo::ApplicationImplBase:
  void OnInitialize() override;
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override;

  // mojo::ContentHandlerFactory::Delegate:
  void RunApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override;

  void ExtractApplication(base::FilePath* extracted_dir,
                          base::FilePath* cache_dir,
                          mojo::URLResponsePtr response,
                          const base::Closure& callback);

  mojo::URLResponseDiskCachePtr url_response_disk_cache_;
  scoped_refptr<base::SingleThreadTaskRunner> handler_task_runner_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(AndroidHandler);
};

bool RegisterAndroidHandlerJni(JNIEnv* env);

}  // namespace shell

#endif  // SHELL_ANDROID_ANDROID_HANDLER_H_
