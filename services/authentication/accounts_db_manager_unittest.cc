// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "accounts_db_manager.h"

#include "base/logging.h"
#include "base/strings/string_tokenizer.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/services/files/interfaces/types.mojom.h"
#include "services/authentication/credentials_impl_db.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace authentication {
namespace {

class AccountsDBTest : public mojo::test::ApplicationTestBase {
 public:
  AccountsDBTest(){};
  ~AccountsDBTest() override{};

 protected:
  void SetUp() override {
    mojo::test::ApplicationTestBase::SetUp();
    mojo::files::FilesPtr files;
    mojo::ConnectToService(shell(), "mojo:files", GetProxy(&files));

    mojo::files::Error error = mojo::files::Error::INTERNAL;
    mojo::files::DirectoryPtr directory;
    files->OpenFileSystem(nullptr, GetProxy(&directory),
                          [&error](mojo::files::Error e) { error = e; });
    CHECK(files.WaitForIncomingResponse());
    CHECK_EQ(mojo::files::Error::OK, error);

    accounts_db_manager_ = new AccountsDbManager(directory.Pass());
  }

  void PopulateCredential(const mojo::String& user, const mojo::String& token) {
    authentication::CredentialsPtr creds = authentication::Credentials::New();
    creds->token = token;
    creds->scopes =
        "https://test_scope_.googleapis.com/auth "
        "https://test_scope_.googleapis.com/profile";
    creds->auth_provider = AuthProvider::GOOGLE;
    creds->credential_type = CredentialType::DOWNSCOPED_OAUTH_REFRESH_TOKEN;
    accounts_db_manager_->UpdateCredentials(user, creds.Pass());
  }

  AccountsDbManager* accountsDBPtr() { return accounts_db_manager_; }

 private:
  AccountsDbManager* accounts_db_manager_;

  DISALLOW_COPY_AND_ASSIGN(AccountsDBTest);
};

TEST_F(AccountsDBTest, CanAddNewAccount) {
  PopulateCredential("new_user", "new_refresh_token");

  mojo::Array<mojo::String> users = accountsDBPtr()->GetAllUsers();
  EXPECT_EQ(1, (int)users.size());
  EXPECT_EQ("new_user", users[0].get());

  authentication::CredentialsPtr creds =
      accountsDBPtr()->GetCredentials("new_user");
  ASSERT_TRUE(!creds->token.is_null());
  EXPECT_EQ("new_refresh_token", creds->token);
}

TEST_F(AccountsDBTest, CanUpdateAnExistingAccount) {
  PopulateCredential("user1", "token1");
  authentication::CredentialsPtr creds =
      accountsDBPtr()->GetCredentials("user1");
  ASSERT_TRUE(!creds->token.is_null());
  EXPECT_EQ("token1", creds->token);

  PopulateCredential("user2", "token2");
  mojo::Array<mojo::String> users = accountsDBPtr()->GetAllUsers();
  EXPECT_EQ(2, (int)users.size());

  // update credential for an existing account
  PopulateCredential("user1", "token3");
  users = accountsDBPtr()->GetAllUsers();
  EXPECT_EQ(2, (int)users.size());

  creds = accountsDBPtr()->GetCredentials("user1");
  ASSERT_TRUE(!creds->token.is_null());
  EXPECT_EQ("token3", creds->token);
}

TEST_F(AccountsDBTest, CanGetCredentials) {
  // No accounts
  authentication::CredentialsPtr creds =
      accountsDBPtr()->GetCredentials("test_user");
  ASSERT_TRUE(creds->token.is_null());

  // Only one account
  PopulateCredential("test_user", "test_refresh_token");
  creds = accountsDBPtr()->GetCredentials("test_user");
  ASSERT_TRUE(!creds->token.is_null());
  EXPECT_EQ("test_refresh_token", creds->token);
  EXPECT_EQ(
      "https://test_scope_.googleapis.com/auth "
      "https://test_scope_.googleapis.com/profile",
      creds->scopes);
  EXPECT_EQ(AuthProvider::GOOGLE, creds->auth_provider);
  EXPECT_EQ(CredentialType::DOWNSCOPED_OAUTH_REFRESH_TOKEN,
            creds->credential_type);

  // Multiple accounts
  PopulateCredential("user31", "token31");
  PopulateCredential("user11", "token11");
  PopulateCredential("user21", "token21");
  creds = accountsDBPtr()->GetCredentials("user11");
  ASSERT_TRUE(!creds->token.is_null());
  EXPECT_EQ("token11", creds->token);

  // For an invalid user
  PopulateCredential("test_user", "test_refresh_token");
  creds = accountsDBPtr()->GetCredentials("test_");
  ASSERT_TRUE(creds->token.is_null());
}

TEST_F(AccountsDBTest, CanGetAllUsers) {
  // No accounts
  mojo::Array<mojo::String> users = accountsDBPtr()->GetAllUsers();
  EXPECT_EQ(0, (int)users.size());

  // More than one account
  PopulateCredential("user1", "token1");
  PopulateCredential("user2", "token2");
  PopulateCredential("user3", "token3");

  users = accountsDBPtr()->GetAllUsers();
  EXPECT_EQ(3, (int)users.size());
}

TEST_F(AccountsDBTest, CanAddNewAuthorization) {
  ASSERT_TRUE(accountsDBPtr()->GetAuthorizedUserForApp("url1").is_null());
  accountsDBPtr()->UpdateAuthorization("url1", "user1");
  EXPECT_EQ(accountsDBPtr()->GetAuthorizedUserForApp("url1").get(), "user1");
}

TEST_F(AccountsDBTest, CanUpdateExistingAuthorization) {
  ASSERT_TRUE(accountsDBPtr()->GetAuthorizedUserForApp("url1").is_null());
  accountsDBPtr()->UpdateAuthorization("url1", "user1");
  EXPECT_EQ(accountsDBPtr()->GetAuthorizedUserForApp("url1").get(), "user1");
  accountsDBPtr()->UpdateAuthorization("url1", "user2");
  EXPECT_EQ(accountsDBPtr()->GetAuthorizedUserForApp("url1").get(), "user2");
}

TEST_F(AccountsDBTest, CanGetAuthorizedUserForInvalidApp) {
  accountsDBPtr()->UpdateAuthorization("url1", "user1");
  ASSERT_TRUE(
      accountsDBPtr()->GetAuthorizedUserForApp("invalid_app_url").is_null());
}

}  // namespace
}  // namespace authentication
