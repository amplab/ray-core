// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_IMPL_H_
#define SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_IMPL_H_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/task_runner.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/url_response_disk_cache/interfaces/url_response_disk_cache.mojom.h"
#include "services/url_response_disk_cache/url_response_disk_cache_db.h"
#include "services/url_response_disk_cache/url_response_disk_cache_delegate.h"

namespace mojo {

class URLResponseDiskCacheImpl : public URLResponseDiskCache {
 public:
  using ResponseFileAndCacheDirCallback =
      base::Callback<void(const base::FilePath&, const base::FilePath&)>;

  // Creates the disk cache database. If |force_clean| is true, or the database
  // is outdated, it will clear the cached content. The actual deletion will be
  // performed using the given task runner, but cache appears as cleared
  // immediately after the function returns.
  static scoped_refptr<URLResponseDiskCacheDB> CreateDB(
      scoped_refptr<base::TaskRunner> task_runner,
      bool force_clean);

  URLResponseDiskCacheImpl(scoped_refptr<base::TaskRunner> task_runner,
                           URLResponseDiskCacheDelegate* delegate,
                           scoped_refptr<URLResponseDiskCacheDB> db,
                           const std::string& remote_application_url,
                           InterfaceRequest<URLResponseDiskCache> request);
  ~URLResponseDiskCacheImpl() override;

 private:
  // URLResponseDiskCache
  void Get(const String& url, const GetCallback& callback) override;
  void Update(URLResponsePtr response) override;
  void Validate(const String& url) override;
  void UpdateAndGet(URLResponsePtr response,
                    const UpdateAndGetCallback& callback) override;
  void UpdateAndGetExtracted(
      URLResponsePtr response,
      const UpdateAndGetExtractedCallback& callback) override;

  // Internal implementation of |UpdateAndGet| using a pair of base::FilePath to
  // represent the paths the the response body and to the per-response client
  // cache directory.
  void UpdateAndGetInternal(URLResponsePtr response,
                            const ResponseFileAndCacheDirCallback& callback);

  // Internal implementation of |UpdateAndGetExtracted|. The parameters are:
  // |callback|: The callback to return values to the caller. It uses FilePath
  //             instead of mojo arrays.
  // |response_body_path|: The path to the content of the body of the response.
  // |consumer_cache_directory|: The directory the user can user to cache its
  //                             own content.
  void UpdateAndGetExtractedInternal(
      const ResponseFileAndCacheDirCallback& callback,
      const base::FilePath& response_body_path,
      const base::FilePath& consumer_cache_directory);

  scoped_refptr<base::TaskRunner> task_runner_;
  std::string request_origin_;
  URLResponseDiskCacheDelegate* delegate_;
  scoped_refptr<URLResponseDiskCacheDB> db_;
  StrongBinding<URLResponseDiskCache> binding_;

  DISALLOW_COPY_AND_ASSIGN(URLResponseDiskCacheImpl);
};

}  // namespace mojo

#endif  // SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_IMPL_H_
