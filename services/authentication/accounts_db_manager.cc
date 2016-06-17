// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/authentication/accounts_db_manager.h"

#include <vector>

#include "base/logging.h"
#include "base/strings/string_tokenizer.h"
#include "mojo/public/cpp/bindings/array.h"
#include "mojo/public/cpp/bindings/type_converter.h"
#include "mojo/services/files/interfaces/files.mojom.h"
#include "services/authentication/authentication_impl_db.mojom.h"
#include "services/authentication/credentials_impl_db.mojom.h"

namespace authentication {

char kAccountsDbFileName[] = "creds_db";
char kAuthDbFileName[] = "auth_db";
const uint32 kAuthDbVersion = 1;
const uint32 kCredsDbVersion = 1;

AccountsDbManager::AccountsDbManager(const mojo::files::DirectoryPtr directory)
    : creds_db_file_(nullptr), auth_db_file_(nullptr) {
  // Initialize in-memory contents from existing DB file
  directory->OpenFile(
      kAccountsDbFileName, GetProxy(&creds_db_file_),
      mojo::files::kOpenFlagCreate | mojo::files::kOpenFlagRead |
          mojo::files::kOpenFlagWrite,
      [this](mojo::files::Error error) {
        if (mojo::files::Error::OK != error) {
          LOG(ERROR) << "Open() error on credentials db:" << error;
          error_ = CREDENTIALS_DB_READ_ERROR;
          return;
        }
      });
  directory->OpenFile(kAuthDbFileName, GetProxy(&auth_db_file_),
                      mojo::files::kOpenFlagCreate |
                          mojo::files::kOpenFlagRead |
                          mojo::files::kOpenFlagWrite,
                      [this](mojo::files::Error error) {
                        if (mojo::files::Error::OK != error) {
                          LOG(ERROR) << "Open() error on auth db:" << error;
                          error_ = AUTHORIZATIONS_DB_READ_ERROR;
                          return;
                        }
                      });

  Initialize();
}

AccountsDbManager::~AccountsDbManager() {}

bool AccountsDbManager::isValid() {
  return error_ == NONE;
}

authentication::CredentialsPtr AccountsDbManager::GetCredentials(
    const mojo::String& username) {
  ensureCredentialsDbInit();
  CHECK(error_ == NONE);

  authentication::CredentialsPtr creds = authentication::Credentials::New();
  if (username.is_null()) {
    return creds.Pass();
  }

  auto it = creds_store_.credentials.find(username);
  if (it != creds_store_.credentials.end()) {
    creds->token = it.GetValue()->token;
    creds->auth_provider = it.GetValue()->auth_provider;
    creds->scopes = it.GetValue()->scopes;
    creds->credential_type = it.GetValue()->credential_type;
  }
  return creds.Pass();
}

mojo::Array<mojo::String> AccountsDbManager::GetAllUsers() {
  ensureCredentialsDbInit();
  CHECK(error_ == NONE);

  mojo::Array<mojo::String> users =
      mojo::Array<mojo::String>::New(creds_store_.credentials.size());
  size_t i = 0;

  for (auto it = creds_store_.credentials.begin();
       it != creds_store_.credentials.end(); it++) {
    users[i++] = it.GetKey().get();
  }

  return users.Pass();
}

void AccountsDbManager::UpdateCredentials(
    const mojo::String& username,
    const authentication::CredentialsPtr creds) {
  ensureCredentialsDbInit();
  CHECK(error_ == NONE);

  if (username.is_null()) {
    return;
  }

  // Update contents cache with new data
  creds_store_.credentials[username] = authentication::Credentials::New();
  creds_store_.credentials[username]->token = creds->token;
  creds_store_.credentials[username]->auth_provider = creds->auth_provider;
  creds_store_.credentials[username]->scopes = creds->scopes;
  creds_store_.credentials[username]->credential_type = creds->credential_type;

  size_t buf_size = creds_store_.GetSerializedSize();
  auto bytes_to_write = mojo::Array<uint8_t>::New(buf_size);
  MOJO_CHECK(creds_store_.Serialize(&bytes_to_write.front(), buf_size));

  mojo::files::Whence whence;
  whence = mojo::files::Whence::FROM_START;
  creds_db_file_->Write(
      bytes_to_write.Pass(), 0, whence,
      [this](mojo::files::Error error, uint32_t num_bytes_written) {
        this->OnCredentialsFileWriteResponse(error, num_bytes_written);
      });
}

void AccountsDbManager::OnCredentialsFileWriteResponse(
    const mojo::files::Error error,
    const uint32_t num_bytes_written) {
  CHECK(error_ == NONE);

  if (mojo::files::Error::OK != error) {
    LOG(ERROR) << "Write() error on accounts db:" << error;
    error_ = CREDENTIALS_DB_WRITE_ERROR;
    return;
  }
}

void AccountsDbManager::ensureCredentialsDbInit() {
  if ((db_init_option_ & CREDENTIALS_DB_INIT_SUCCESS) !=
      CREDENTIALS_DB_INIT_SUCCESS) {
    CHECK(creds_db_file_.WaitForIncomingResponse());
  }
}

void AccountsDbManager::ensureAuthorizationsDbInit() {
  if ((db_init_option_ & AUTHORIZATIONS_DB_INIT_SUCCESS) !=
      AUTHORIZATIONS_DB_INIT_SUCCESS) {
    CHECK(auth_db_file_.WaitForIncomingResponse());
  }
}

void AccountsDbManager::Initialize() {
  CHECK(error_ == NONE);

  const size_t kMaxReadSize = 1 * 1024 * 1024;
  mojo::Array<uint8_t> cred_bytes_read;
  creds_db_file_->Read(
      kMaxReadSize - 1, 0, mojo::files::Whence::FROM_START,
      [this](mojo::files::Error error, mojo::Array<uint8_t> cred_bytes_read) {
        this->OnCredentialsFileReadResponse(error, cred_bytes_read.Pass());
      });

  mojo::Array<uint8_t> auth_bytes_read;
  auth_db_file_->Read(
      kMaxReadSize - 1, 0, mojo::files::Whence::FROM_START,
      [this](mojo::files::Error error, mojo::Array<uint8_t> auth_bytes_read) {
        this->OnAuthorizationsFileReadResponse(error, auth_bytes_read.Pass());
      });
}

void AccountsDbManager::OnCredentialsFileReadResponse(
    const mojo::files::Error error,
    const mojo::Array<uint8_t> bytes_read) {
  CHECK(error_ == NONE);

  if (error != mojo::files::Error::OK) {
    LOG(ERROR) << "Read() error on accounts db: " << error;
    error_ = CREDENTIALS_DB_READ_ERROR;
    return;
  }

  if (bytes_read.size() != 0) {
    // Deserialize data from file
    const char* data = reinterpret_cast<const char*>(&bytes_read[0]);
    if (!creds_store_.Deserialize((void*)data, bytes_read.size())) {
      LOG(ERROR) << "Deserialize() error on accounts db.";
      error_ = CREDENTIALS_DB_VALIDATE_ERROR;
      return;
    }
    // When we have multiple versions, this is not a fatal error, but a sign
    // that we need to update (or reinitialize) the db.
    CHECK_EQ(creds_store_.version, kCredsDbVersion);
  } else {
    creds_store_.version = kCredsDbVersion;
  }

  db_init_option_ |= CREDENTIALS_DB_INIT_SUCCESS;
}

void AccountsDbManager::OnAuthorizationsFileReadResponse(
    const mojo::files::Error error,
    const mojo::Array<uint8_t> bytes_read) {
  CHECK(error_ == NONE);

  if (error != mojo::files::Error::OK) {
    LOG(ERROR) << "Read() error on auth db: " << error;
    error_ = AUTHORIZATIONS_DB_READ_ERROR;
    return;
  }

  if (bytes_read.size() != 0) {
    // Deserialize data from file
    const char* data = reinterpret_cast<const char*>(&bytes_read[0]);
    if (!auth_grants_.Deserialize((void*)data, bytes_read.size())) {
      LOG(ERROR) << "Deserialize() error on auth db.";
      error_ = AUTHORIZATIONS_DB_VALIDATE_ERROR;
      return;
    }

    // When we have multiple versions, this is not a fatal error, but a sign
    // that we need to update (or reinitialize) the db.
    CHECK_EQ(auth_grants_.version, kAuthDbVersion);
  } else {
    auth_grants_.version = kAuthDbVersion;
  }

  db_init_option_ |= AUTHORIZATIONS_DB_INIT_SUCCESS;
}

mojo::String AccountsDbManager::GetAuthorizedUserForApp(mojo::String app_url) {
  ensureAuthorizationsDbInit();
  CHECK(error_ == NONE);

  if (app_url.is_null()) {
    return nullptr;
  }
  auto it = auth_grants_.last_selected_accounts.find(app_url);
  if (it == auth_grants_.last_selected_accounts.end()) {
    return nullptr;
  }
  return mojo::String(it.GetValue());
}

void AccountsDbManager::UpdateAuthorization(mojo::String app_url,
                                            mojo::String username) {
  ensureAuthorizationsDbInit();
  CHECK(error_ == NONE);

  if (app_url.is_null() || username.is_null()) {
    return;
  }
  auth_grants_.last_selected_accounts[app_url] = username;

  size_t buf_size = auth_grants_.GetSerializedSize();
  auto bytes_to_write = mojo::Array<uint8_t>::New(buf_size);
  MOJO_CHECK(auth_grants_.Serialize(&bytes_to_write.front(), buf_size));

  mojo::files::Whence whence;
  whence = mojo::files::Whence::FROM_START;
  auth_db_file_->Write(
      bytes_to_write.Pass(), 0, whence,
      [this](mojo::files::Error error, uint32_t num_bytes_written) {
        this->OnAuthorizationsFileWriteResponse(error, num_bytes_written);
      });
}

void AccountsDbManager::OnAuthorizationsFileWriteResponse(
    const mojo::files::Error error,
    const uint32_t num_bytes_written) {
  CHECK(error_ == NONE);

  if (mojo::files::Error::OK != error) {
    LOG(ERROR) << "Write() error on auth db:" << error;
    error_ = AUTHORIZATIONS_DB_WRITE_ERROR;
    return;
  }
}

}  // namespace authentication
