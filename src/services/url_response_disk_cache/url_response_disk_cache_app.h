// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_APP_H_
#define SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_APP_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/task_runner.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/services/url_response_disk_cache/interfaces/url_response_disk_cache.mojom.h"
#include "services/url_response_disk_cache/url_response_disk_cache_db.h"
#include "services/url_response_disk_cache/url_response_disk_cache_delegate.h"

namespace mojo {

class URLResponseDiskCacheApp : public ApplicationImplBase {
 public:
  explicit URLResponseDiskCacheApp(scoped_refptr<base::TaskRunner> task_runner,
                                   URLResponseDiskCacheDelegate* delegate);
  ~URLResponseDiskCacheApp() override;

 private:
  // ApplicationImplBase:
  void OnInitialize() override;
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override;
  void Terminate(MojoResult result) override;

  scoped_refptr<base::TaskRunner> task_runner_;
  scoped_refptr<URLResponseDiskCacheDB> db_;
  URLResponseDiskCacheDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(URLResponseDiskCacheApp);
};

}  // namespace mojo

#endif  // SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_APP_H_
