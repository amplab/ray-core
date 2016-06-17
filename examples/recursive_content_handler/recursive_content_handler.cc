// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/content_handler/interfaces/content_handler.mojom.h"

namespace mojo {
namespace examples {

class RecursiveContentHandler : public ApplicationImplBase,
                                public ContentHandlerFactory::ManagedDelegate {
 public:
  RecursiveContentHandler() {}

 private:
  // Overridden from ApplicationImplBase:
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<ContentHandler>(
        ContentHandlerFactory::GetInterfaceRequestHandler(this));
    return true;
  }

  // Overridden from ContentHandlerFactory::ManagedDelegate:
  std::unique_ptr<ContentHandlerFactory::HandledApplicationHolder>
  CreateApplication(InterfaceRequest<Application> application_request,
                    URLResponsePtr response) override {
    LOG(INFO) << "RecursiveContentHandler called with url: " << response->url;
    auto app = new RecursiveContentHandler();
    app->Bind(application_request.Pass());
    return make_handled_factory_holder(app);
  }

  DISALLOW_COPY_AND_ASSIGN(RecursiveContentHandler);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  mojo::examples::RecursiveContentHandler recursive_content_handler;
  return mojo::RunApplication(application_request, &recursive_content_handler);
}
