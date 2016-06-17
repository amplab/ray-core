// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/bank_app/bank.mojom.h"
#include "mojo/common/binding_set.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/services/vanadium/security/interfaces/principal.mojom.h"

namespace examples {

using vanadium::PrincipalServicePtr;

class BankImpl : public Bank {
 public:
  BankImpl() : balance_(0) {}
  void Deposit(int32_t usd) override { balance_ += usd; }
  void Withdraw(int32_t usd) override { balance_ -= usd; }
  void GetBalance(const GetBalanceCallback& callback) override {
    callback.Run(balance_);
  }
 private:
  int32_t balance_;
};

class BankUser {
 public:
  explicit BankUser(std::string* user) : user_(user) { }
  void Run(const vanadium::UserPtr& user) const {
    user_->clear();
    if (user) {
      *user_ = user->email;
    }
  }
 private:
  std::string *user_;
};

class BankApp : public mojo::ApplicationImplBase {
 public:
  BankApp() {}

  // ApplicationImplBase overrides:
  void OnInitialize() override {
    mojo::ConnectToService(shell(), "mojo:principal_service",
                           GetProxy(&login_service_));
  }

  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    std::string url = service_provider_impl->connection_context().remote_url;
    if (url.length() > 0) {
      vanadium::AppInstanceNamePtr app(vanadium::AppInstanceName::New());
      app->url = url;
      std::string user;
      login_service_->GetUser(app.Pass(), BankUser(&user));
      // Check and see whether we got a valid user blessing.
      if (!login_service_.WaitForIncomingResponse()) {
        MOJO_LOG(INFO) << "Failed to get a valid user blessing";
        return false;
      }
      // Record user access to the bank and reject customers that
      // don't have a user identity.
      if (user.empty()) {
        MOJO_LOG(INFO) << "Rejecting customer without a user identity";
        return false;
      }
      MOJO_LOG(INFO) << "Customer " << user << " accessing bank";
    }
    service_provider_impl->AddService<Bank>(
        [this](const mojo::ConnectionContext& connection_context,
               mojo::InterfaceRequest<Bank> bank_request) {
          bindings_.AddBinding(&bank_impl_, bank_request.Pass());
        });
    return true;
  }

 private:
  BankImpl bank_impl_;
  mojo::BindingSet<Bank> bindings_;
  vanadium::PrincipalServicePtr login_service_;
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  examples::BankApp bank_app;
  return mojo::RunApplication(application_request, &bank_app);
}
