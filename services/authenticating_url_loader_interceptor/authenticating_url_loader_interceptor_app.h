// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_APP_H_
#define SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_APP_H_

#include "base/macros.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/services/authenticating_url_loader_interceptor/interfaces/authenticating_url_loader_interceptor_meta_factory.mojom.h"
#include "services/authenticating_url_loader_interceptor/authenticating_url_loader_interceptor_meta_factory_impl.h"

namespace mojo {

class AuthenticatingURLLoaderInterceptorApp : public ApplicationImplBase {
 public:
  AuthenticatingURLLoaderInterceptorApp();
  ~AuthenticatingURLLoaderInterceptorApp() override;

 private:
  // ApplicationImplBase override:
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override;

  // Cache received tokens per origin of the connecting app and origin of the
  // loaded URL so that once a token has been requested it is not necessary to
  // do nultiple http connections to retrieve additional resources on the same
  // host.
  std::map<GURL, std::map<GURL, std::string>> tokens_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatingURLLoaderInterceptorApp);
};

}  // namespace mojo

#endif  // SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_APP_H_
