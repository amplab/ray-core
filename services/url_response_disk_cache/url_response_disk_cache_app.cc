// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/url_response_disk_cache/url_response_disk_cache_app.h"

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "services/url_response_disk_cache/url_response_disk_cache_impl.h"

namespace mojo {

URLResponseDiskCacheApp::URLResponseDiskCacheApp(
    scoped_refptr<base::TaskRunner> task_runner,
    URLResponseDiskCacheDelegate* delegate)
    : task_runner_(task_runner), delegate_(delegate) {}

URLResponseDiskCacheApp::~URLResponseDiskCacheApp() {}

void URLResponseDiskCacheApp::OnInitialize() {
  base::CommandLine command_line(args());
  bool force_clean = command_line.HasSwitch("clear");
  db_ = URLResponseDiskCacheImpl::CreateDB(task_runner_, force_clean);
}

bool URLResponseDiskCacheApp::OnAcceptConnection(
    ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<URLResponseDiskCache>([this](
      const ConnectionContext& connection_context,
      InterfaceRequest<URLResponseDiskCache> request) {
    new URLResponseDiskCacheImpl(task_runner_, delegate_, db_,
                                 connection_context.remote_url, request.Pass());
  });
  return true;
}

void URLResponseDiskCacheApp::Terminate(MojoResult result) {
  // TODO(vtl): This "app" is not a normal app. Instead, it is run as part of
  // the shell, using a |base::MessageLoop| that's owned by the shell (ugh!).
  if (base::MessageLoop::current()->is_running())
    base::MessageLoop::current()->Quit();
}

}  // namespace mojo
