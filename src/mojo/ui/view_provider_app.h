// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_UI_VIEW_PROVIDER_APP_H_
#define MOJO_UI_VIEW_PROVIDER_APP_H_

#include <string>

#include "mojo/common/strong_binding_set.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/ui/views/interfaces/view_provider.mojom.h"

namespace mojo {

class ServiceProviderImpl;

namespace ui {

// Abstract implementation of a simple application that offers a ViewProvider.
// Subclasses must provide a function to create the necessary Views.
//
// It is not necessary to use this class to implement all ViewProviders.
// This class is merely intended to make the simple apps easier to write.
class ViewProviderApp : public ApplicationImplBase {
 public:
  ViewProviderApp();
  ~ViewProviderApp() override;

  // |ApplicationImplBase|:
  void OnInitialize() override;
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override;

  // Called by the ViewProvider to create a view.
  // This method may be called multiple times in the case where the
  // view provider is asked to create multiple view instances.
  //
  // The |view_provider_url| is the connection URL of the view provider request.
  //
  // The |view_owner_request| should be attached to the newly created view
  // and closed or left pending if the view could not be created.
  //
  // The |services| parameter is used to receive services from the view
  // on behalf of the caller.
  virtual void CreateView(const std::string& view_provider_url,
                          InterfaceRequest<ViewOwner> view_owner_request,
                          InterfaceRequest<ServiceProvider> services) = 0;

 private:
  class DelegatingViewProvider;

  void CreateView(DelegatingViewProvider* provider,
                  const std::string& view_provider_url,
                  InterfaceRequest<ViewOwner> view_owner_request,
                  InterfaceRequest<ServiceProvider> services);

  mojo::TracingImpl tracing_;
  StrongBindingSet<ViewProvider> bindings_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ViewProviderApp);
};

}  // namespace ui
}  // namespace mojo

#endif  // MOJO_UI_VIEW_PROVIDER_APP_H_
