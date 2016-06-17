// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_URL_RESPONSE_DISK_CACHE_DELEGATE_IMPL_H_
#define SHELL_ANDROID_URL_RESPONSE_DISK_CACHE_DELEGATE_IMPL_H_

#include <map>

#include "base/android/scoped_java_ref.h"
#include "services/url_response_disk_cache/url_response_disk_cache_delegate.h"
#include "shell/context.h"

struct AAssetManager;

namespace shell {

class URLResponseDiskCacheDelegateImpl
    : public mojo::URLResponseDiskCacheDelegate {
 public:
  URLResponseDiskCacheDelegateImpl(Context* contest, jobject j_asset_manager);
  ~URLResponseDiskCacheDelegateImpl() override;

 private:
  // mojo:URLResponseDiskCacheDelegate implementation:
  void GetInitialPath(scoped_refptr<base::TaskRunner> task_runner,
                      const std::string& url,
                      const base::FilePath& target,
                      const base::Callback<void(bool)>& callback) override;

  bool ExtractAsset(const std::string& asset_name,
                    const base::FilePath& target);

  void PopulateAssetsIfNeeded();

  Context* context_;
  base::android::ScopedJavaGlobalRef<jobject> j_asset_manager_;
  std::map<std::string, std::string> url_to_asset_name_;
  AAssetManager* asset_manager_;

  DISALLOW_COPY_AND_ASSIGN(URLResponseDiskCacheDelegateImpl);
};

}  // namespace shell

#endif  // SHELL_ANDROID_URL_RESPONSE_DISK_CACHE_DELEGATE_IMPL_H_
