// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/url_response_disk_cache_delegate_impl.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "base/android/jni_android.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file.h"
#include "base/strings/string_util.h"
#include "base/task_runner_util.h"

namespace shell {

namespace {
const char kMojoApplicationSuffix[] = ".mojo";
}  // namespace

URLResponseDiskCacheDelegateImpl::URLResponseDiskCacheDelegateImpl(
    Context* context,
    jobject j_asset_manager)
    : context_(context),
      j_asset_manager_(base::android::AttachCurrentThread(), j_asset_manager),
      asset_manager_(
          AAssetManager_fromJava(base::android::AttachCurrentThread(),
                                 j_asset_manager_.obj())) {}

URLResponseDiskCacheDelegateImpl::~URLResponseDiskCacheDelegateImpl() {}

void URLResponseDiskCacheDelegateImpl::GetInitialPath(
    scoped_refptr<base::TaskRunner> task_runner,
    const std::string& url,
    const base::FilePath& target,
    const base::Callback<void(bool)>& callback) {
  PopulateAssetsIfNeeded();
  if (url_to_asset_name_.find(url) == url_to_asset_name_.end()) {
    callback.Run(false);
    return;
  }

  PostTaskAndReplyWithResult(
      task_runner.get(), FROM_HERE,
      base::Bind(&URLResponseDiskCacheDelegateImpl::ExtractAsset,
                 base::Unretained(this), url_to_asset_name_[url], target),
      callback);
}

bool URLResponseDiskCacheDelegateImpl::ExtractAsset(
    const std::string& asset_name,
    const base::FilePath& target) {
  AAsset* asset = AAssetManager_open(asset_manager_, asset_name.c_str(),
                                     AASSET_MODE_STREAMING);
  if (!asset)
    return false;
  base::ScopedClosureRunner cleanup(
      base::Bind(&AAsset_close, base::Unretained(asset)));
  base::File target_file(target,
                         base::File::FLAG_CREATE | base::File::FLAG_WRITE);
  if (!target_file.IsValid())
    return false;
  char buffer[4096];
  int nb_read;
  do {
    nb_read = AAsset_read(asset, buffer, arraysize(buffer));
    if (nb_read > 0) {
      if (target_file.WriteAtCurrentPos(buffer, nb_read) != nb_read)
        return false;
    }
  } while (nb_read > 0);
  return nb_read == 0;
}

void URLResponseDiskCacheDelegateImpl::PopulateAssetsIfNeeded() {
  if (url_to_asset_name_.empty()) {
    AAssetDir* dir = AAssetManager_openDir(asset_manager_, "");
    if (!dir)
      return;
    base::ScopedClosureRunner cleanup(
        base::Bind(&AAssetDir_close, base::Unretained(dir)));
    while (const char* filename = AAssetDir_getNextFileName(dir)) {
      std::string file_name_string = filename;
      if (base::EndsWith(file_name_string, kMojoApplicationSuffix,
                         base::CompareCase::SENSITIVE)) {
        std::string base_name = file_name_string.substr(
            0,
            file_name_string.size() - (arraysize(kMojoApplicationSuffix) - 1));
        std::string url = context_->url_resolver()
                              ->ResolveMojoURL(GURL("mojo:" + base_name))
                              .spec();
        url_to_asset_name_[url] = file_name_string;
      }
    }
  }
}

}  // namespace shell
