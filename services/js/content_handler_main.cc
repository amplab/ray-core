// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/i18n/icu_util.h"
#include "gin/array_buffer.h"
#include "gin/public/isolate_holder.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "services/js/js_app.h"

namespace js {

class JsContentHandler : public mojo::ApplicationImplBase,
                         public mojo::ContentHandlerFactory::ManagedDelegate {
 public:
  JsContentHandler() {}

 private:
  // Overridden from mojo::ApplicationImplBase:
  void OnInitialize() override {
    static const char v8Flags[] = "--harmony-classes";
    v8::V8::SetFlagsFromString(v8Flags, sizeof(v8Flags) - 1);
    base::i18n::InitializeICU();
    gin::IsolateHolder::Initialize(gin::IsolateHolder::kStrictMode,
                                   gin::ArrayBufferAllocator::SharedInstance());
  }

  // Overridden from mojo::ApplicationImplBase:
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<mojo::ContentHandler>(
        mojo::ContentHandlerFactory::GetInterfaceRequestHandler(this));
    return true;
  }

  // Overridden from mojo::ContentHandlerFactory::ManagedDelegate:
  std::unique_ptr<mojo::ContentHandlerFactory::HandledApplicationHolder>
  CreateApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override {
    return std::unique_ptr<
        mojo::ContentHandlerFactory::HandledApplicationHolder>(
        new JSApp(application_request.Pass(), response.Pass()));
  }

  DISALLOW_COPY_AND_ASSIGN(JsContentHandler);
};

}  // namespace js

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  js::JsContentHandler js_content_handler;
  return mojo::RunApplication(application_request, &js_content_handler);
}
