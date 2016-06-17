// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/common/binding_set.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/authentication/interfaces/authentication.mojom.h"
#include "mojo/services/authentication/interfaces/authentication_admin.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "services/authentication/google_authentication_admin_impl.h"
#include "services/authentication/google_authentication_impl.h"

namespace authentication {

class GoogleAccountManagerApp : public mojo::ApplicationImplBase {
 public:
  GoogleAccountManagerApp() {}
  ~GoogleAccountManagerApp() override {}

  void OnInitialize() override {
    mojo::ConnectToService(shell(), "mojo:network_service",
                           GetProxy(&network_service_));
    mojo::ConnectToService(shell(), "mojo:files", GetProxy(&files_));
  }

  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<AuthenticationAdminService>(
        [this](const mojo::ConnectionContext& connection_context,
               mojo::InterfaceRequest<AuthenticationAdminService> request) {
          mojo::files::Error error = mojo::files::Error::INTERNAL;
          mojo::files::DirectoryPtr directory;
          files_->OpenFileSystem("app_persistent_cache", GetProxy(&directory),
                                 [&error](mojo::files::Error e) { error = e; });
          CHECK(files_.WaitForIncomingResponse());
          if (mojo::files::Error::OK != error) {
            LOG(FATAL) << "Unable to initialize accounts DB";
          }
          new authentication::GoogleAuthenticationAdminServiceImpl(
              request.Pass(), url(), network_service_, directory);
        });
    service_provider_impl->AddService<AuthenticationService>(
        [this](const mojo::ConnectionContext& connection_context,
               mojo::InterfaceRequest<AuthenticationService> request) {
          mojo::files::Error error = mojo::files::Error::INTERNAL;
          mojo::files::DirectoryPtr directory;
          files_->OpenFileSystem("app_persistent_cache", GetProxy(&directory),
                                 [&error](mojo::files::Error e) { error = e; });
          CHECK(files_.WaitForIncomingResponse());
          if (mojo::files::Error::OK != error) {
            LOG(FATAL) << "Unable to initialize accounts DB";
          }
          new authentication::GoogleAuthenticationServiceImpl(
              request.Pass(), url(), network_service_, directory);
        });
    return true;
  }

 private:
  mojo::NetworkServicePtr network_service_;
  mojo::files::FilesPtr files_;

  DISALLOW_COPY_AND_ASSIGN(GoogleAccountManagerApp);
};

}  // namespace authentication

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  authentication::GoogleAccountManagerApp google_account_manager_app;
  return mojo::RunApplication(application_request, &google_account_manager_app);
}
