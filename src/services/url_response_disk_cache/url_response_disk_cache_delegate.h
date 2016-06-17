// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_DELEGATE_H_
#define SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_DELEGATE_H_

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/task_runner.h"

namespace mojo {

// This class allows to customized the behavior of the |URLResponseDiskCache|
// implementation.
class URLResponseDiskCacheDelegate {
 public:
  URLResponseDiskCacheDelegate() {}
  virtual ~URLResponseDiskCacheDelegate() {}

  // Allows to prefill the cache with content. This method will be called by the
  // cache when an item that has never been in the cache is requested.
  // |task_runner| is a task runner that can be used to do I/O. |url| is the url
  // of the entry. |target| is a path where the content must be copied to if it
  // exists. |callback| must be called with |true| if the delegate can retrieve
  // an initial content for |url| after having push the content to |target|,
  // otherwise, |callback| must be called with |false|. |callback| can be called
  // before this method returns.
  virtual void GetInitialPath(scoped_refptr<base::TaskRunner> task_runner,
                              const std::string& url,
                              const base::FilePath& target,
                              const base::Callback<void(bool)>& callback) = 0;

  DISALLOW_COPY_AND_ASSIGN(URLResponseDiskCacheDelegate);
};

}  // namespace mojo

#endif  // SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_DELEGATE_H_
