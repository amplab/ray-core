// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_APPLICATION_MANAGER_NETWORK_FETCHER_H_
#define SHELL_APPLICATION_MANAGER_NETWORK_FETCHER_H_

#include "shell/application_manager/fetcher.h"

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "mojo/services/network/interfaces/url_loader.mojom.h"
#include "mojo/services/url_response_disk_cache/interfaces/url_response_disk_cache.mojom.h"
#include "url/gurl.h"

namespace shell {

// Implements Fetcher for http[s] files.
class NetworkFetcher : public Fetcher {
 public:
  NetworkFetcher(bool disable_cache,
                 bool force_offline_by_default,
                 const GURL& url,
                 mojo::URLResponseDiskCache* url_response_disk_cache,
                 mojo::NetworkService* network_service,
                 const FetchCallback& loader_callback);

  ~NetworkFetcher() override;

 private:
  const GURL& GetURL() const override;
  GURL GetRedirectURL() const override;

  mojo::URLResponsePtr AsURLResponse(base::TaskRunner* task_runner,
                                     uint32_t skip) override;

  void AsPath(
      base::TaskRunner* task_runner,
      base::Callback<void(const base::FilePath&, bool)> callback) override;

  std::string MimeType() override;

  bool HasMojoMagic() override;

  bool PeekFirstLine(std::string* line) override;

  // Returns whether the content can be loaded directly from cache. Local hosts
  // are not loaded from cache to allow effective development.
  bool CanLoadDirectlyFromCache();

  // Tries to load the URL directly from the offline cache.
  void LoadFromCache();

  // Callback from the offline cache.
  void OnResponseReceived(bool schedule_update,
                          mojo::URLResponsePtr response,
                          mojo::Array<uint8_t> path_as_array,
                          mojo::Array<uint8_t> cache_dir);

  // Start a network request to load the URL.
  void StartNetworkRequest();

  // Callback from the network stack.
  void OnLoadComplete(mojo::URLResponsePtr response);

  // Callback from the offline cache when the response is saved by the cache.
  void OnFileSavedToCache(mojo::URLResponsePtr response,
                          mojo::Array<uint8_t> path_as_array,
                          mojo::Array<uint8_t> cache_dir);

  // Record the mapping from URLs to files.
  static void RecordCacheToURLMapping(const base::FilePath& path,
                                      const GURL& url);

  const bool disable_cache_;
  const bool force_offline_by_default_;
  const GURL url_;
  mojo::URLResponseDiskCache* url_response_disk_cache_;
  mojo::NetworkService* network_service_;
  mojo::URLLoaderPtr url_loader_;
  mojo::URLResponsePtr response_;
  base::FilePath path_;
  base::WeakPtrFactory<NetworkFetcher> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkFetcher);
};

}  // namespace shell

#endif  // SHELL_APPLICATION_MANAGER_NETWORK_FETCHER_H_
