// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/bank_app/bank.mojom.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/services/vanadium/security/interfaces/principal.mojom.h"

namespace examples {

class LoginHandler {
 public:
  void Run(const vanadium::UserPtr& user) const {
    if (user) {
      MOJO_LOG(INFO) << "User logged-in as " << user->email;
    }
  }
};

class BankCustomer : public mojo::ApplicationImplBase {
 public:
  void OnInitialize() override {
    mojo::ConnectToService(shell(), "mojo:principal_service",
                           GetProxy(&login_service_));

    // Login to the principal service to get a user identity.
    login_service_->Login(LoginHandler());
    // Check and see whether we got a valid user id.
    if (!login_service_.WaitForIncomingResponse()) {
      MOJO_LOG(INFO) << "Login() to the principal service failed";
    }

    BankPtr bank;
    mojo::ConnectToService(shell(), "mojo:bank", GetProxy(&bank));
    bank->Deposit(500/*usd*/);
    bank->Withdraw(100/*usd*/);
    auto gb_callback = [](const int32_t& balance) {
      MOJO_LOG(INFO) << "Bank balance: " << balance;
    };
    bank->GetBalance(mojo::Callback<void(const int32_t&)>(gb_callback));
    bank.WaitForIncomingResponse();
  }
  void OnQuit() override { login_service_->Logout(); }

 private:
  vanadium::PrincipalServicePtr login_service_;
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  examples::BankCustomer bank_customer;
  return mojo::RunApplication(application_request, &bank_customer);
}
