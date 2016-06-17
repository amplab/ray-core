// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/url_response_disk_cache_loader.h"

namespace shell {

URLResponseDiskCacheLoader::URLResponseDiskCacheLoader(
    scoped_refptr<base::TaskRunner> task_runner,
    mojo::URLResponseDiskCacheDelegate* delegate)
    : url_response_disk_cache_app_(task_runner, delegate) {}

URLResponseDiskCacheLoader::~URLResponseDiskCacheLoader() {}

void URLResponseDiskCacheLoader::Load(
    const GURL& url,
    mojo::InterfaceRequest<mojo::Application> application_request) {
  DCHECK(application_request.is_pending());
  url_response_disk_cache_app_.Bind(application_request.Pass());
}

}  // namespace shell
