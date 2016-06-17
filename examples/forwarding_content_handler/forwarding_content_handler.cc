// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/macros.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/services/content_handler/interfaces/content_handler.mojom.h"

namespace mojo {
namespace examples {

class ForwardingApplicationImpl : public Application {
 public:
  ForwardingApplicationImpl(InterfaceRequest<Application> request,
                            std::string target_url)
      : binding_(this, request.Pass()),
        target_url_(target_url) {
  }

 private:
  // Application:
  void Initialize(InterfaceHandle<Shell> shell,
                  Array<String> args,
                  const mojo::String& url) override {
    shell_ = ShellPtr::Create(std::move(shell));
  }
  void AcceptConnection(const String& requestor_url,
                        const String& requested_url,
                        InterfaceRequest<ServiceProvider> services) override {
    shell_->ConnectToApplication(target_url_, services.Pass());
  }
  void RequestQuit() override {
    RunLoop::current()->Quit();
  }

  Binding<Application> binding_;
  std::string target_url_;
  ShellPtr shell_;
};

class ForwardingContentHandler : public ApplicationImplBase,
                                 public ContentHandlerFactory::ManagedDelegate {
 public:
  ForwardingContentHandler() {}

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
    CHECK(!response.is_null());
    const std::string requestor_url(response->url);
    std::string target_url;
    if (!common::BlockingCopyToString(response->body.Pass(), &target_url)) {
      LOG(WARNING) << "unable to read target URL from " << requestor_url;
      return nullptr;
    }
    return make_handled_factory_holder(
        new ForwardingApplicationImpl(application_request.Pass(), target_url));
  }

  DISALLOW_COPY_AND_ASSIGN(ForwardingContentHandler);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  mojo::examples::ForwardingContentHandler forwarding_content_handler;
  return mojo::RunApplication(application_request, &forwarding_content_handler);
}
