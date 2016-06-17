// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/js/js_app.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "gin/converter.h"
#include "gin/modules/module_registry.h"
#include "gin/try_catch.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/js/js_app_message_loop_observers.h"
#include "services/js/system/core.h"
#include "services/js/system/handle.h"
#include "services/js/system/support.h"

namespace js {

const char JSApp::kMainModuleName[] = "main";

JSApp::JSApp(mojo::InterfaceRequest<mojo::Application> application_request,
             mojo::URLResponsePtr response)
    : application_request_(application_request.Pass()) {
  v8::Isolate* isolate = isolate_holder_.isolate();
  message_loop_observers_.reset(new JSAppMessageLoopObservers(isolate));

  DCHECK(!response.is_null());
  std::string url(response->url);
  std::string source;
  CHECK(mojo::common::BlockingCopyToString(response->body.Pass(), &source));

  shell_runner_.reset(new gin::ShellRunner(&runner_delegate_, isolate));
  gin::Runner::Scope scope(shell_runner_.get());
  shell_runner_->Run(source.c_str(), kMainModuleName);

  gin::ModuleRegistry* registry =
      gin::ModuleRegistry::From(shell_runner_->GetContextHolder()->context());
  registry->LoadModule(
      isolate,
      kMainModuleName,
      base::Bind(&JSApp::OnAppLoaded, base::Unretained(this), url));
}

JSApp::~JSApp() {
  app_instance_.Reset();
}

void JSApp::OnAppLoaded(std::string url, v8::Handle<v8::Value> main_module) {
  gin::Runner::Scope scope(shell_runner_.get());
  gin::TryCatch try_catch;
  v8::Isolate* isolate = isolate_holder_.isolate();

  v8::Handle<v8::Value> argv[] = {
      gin::ConvertToV8(isolate,
                       application_request_.PassMessagePipe().release()),
      gin::ConvertToV8(isolate, url)};

  v8::Handle<v8::Function> app_class;
  CHECK(gin::ConvertFromV8(isolate, main_module, &app_class));
  app_instance_.Reset(isolate, app_class->NewInstance(arraysize(argv), argv));
  if (try_catch.HasCaught())
    runner_delegate_.UnhandledException(shell_runner_.get(), try_catch);
}

}  // namespace js

