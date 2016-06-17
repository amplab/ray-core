// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATION_ACCOUNTS_DB_MANAGER_H_
#define SERVICES_AUTHENTICATION_ACCOUNTS_DB_MANAGER_H_

#include <type_traits>

#include "base/macros.h"
#include "mojo/services/files/interfaces/files.mojom.h"
#include "services/authentication/authentication_impl_db.mojom.h"
#include "services/authentication/credentials_impl_db.mojom.h"

namespace authentication {

// Implementation of user account management service on systems like FNL. This
// uses native mojo files service as the underlying mechanism to store user
// credentials and supports operations such as to add a new user account, update
// existing user credentials and fetching current credentials for a given user.
class AccountsDbManager {
 public:
  // Constructor that takes a directory handle to create or open existing auth
  // and credentials db files. It also initializes the in-memory cache with the
  // file contents from the disk.
  AccountsDbManager(const mojo::files::DirectoryPtr directory);
  ~AccountsDbManager();
  // Returns true, if there are no errors during Db initialization or commits.
  bool isValid();
  // Fetches auth credentials for a given user account.
  authentication::CredentialsPtr GetCredentials(const mojo::String& username);
  // Returns a list of all users registered on the device.
  mojo::Array<mojo::String> GetAllUsers();
  // Updates or adds new auth credentials for a given user account.
  void UpdateCredentials(const mojo::String& username,
                         const authentication::CredentialsPtr creds);
  // Returns previously used account name for the given application or null if
  // not found.
  mojo::String GetAuthorizedUserForApp(mojo::String app_url);
  // Updates the grants database for the given application and username.
  void UpdateAuthorization(mojo::String app_url, mojo::String username);

 private:
  // DB Init Options for tracking the init state of in-memory contents.
  enum DBInitOptions {
    // Default init state.
    START_INIT = 0x01,
    // Credentials db in-memory content is populated successfully.
    CREDENTIALS_DB_INIT_SUCCESS = 0x02,
    // Authorizations db in-memory content is populated successfully.
    AUTHORIZATIONS_DB_INIT_SUCCESS = 0x04
  };

  // Errors for tracking the lifecycle of AccountsDbManager object.
  enum Error {
    NONE = 1,
    // File read error on credentials db.
    CREDENTIALS_DB_READ_ERROR,
    // File write error on credentials db.
    CREDENTIALS_DB_WRITE_ERROR,
    // File validate error on credentials db.
    CREDENTIALS_DB_VALIDATE_ERROR,
    // File read error on authorizations db.
    AUTHORIZATIONS_DB_READ_ERROR,
    // File write error on authorizations db.
    AUTHORIZATIONS_DB_WRITE_ERROR,
    // File validate error on authorizations db.
    AUTHORIZATIONS_DB_VALIDATE_ERROR
  };

  // Populates contents with existing user credentials.
  void Initialize();
  // Reads from credentials file and populates in-memory contents cache.
  void OnCredentialsFileReadResponse(const mojo::files::Error error,
                                     const mojo::Array<uint8_t> bytes_read);
  // Parses response from credentials file write operation
  void OnCredentialsFileWriteResponse(const mojo::files::Error error,
                                      const uint32_t num_bytes_written);
  // Reads from auth file and populates in-memory grants cache.
  void OnAuthorizationsFileReadResponse(const mojo::files::Error error,
                                        const mojo::Array<uint8_t> bytes_read);
  // Parses response from auth file write operation
  void OnAuthorizationsFileWriteResponse(const mojo::files::Error error,
                                         const uint32_t num_bytes_written);
  // Waits for the credentials db file handle to finish the blocking read
  // operation as part of initialization, before proceeding further.
  void ensureCredentialsDbInit();
  // Waits for the authorizations db file handle to finish the blocking read
  // operation as part of initialization, before proceeding further.
  void ensureAuthorizationsDbInit();

  // File pointer to the stored account credentials db file.
  mojo::files::FilePtr creds_db_file_;
  // File pointer to the list of authorized modules.
  mojo::files::FilePtr auth_db_file_;
  // In-memory store for credential data for all users.
  authentication::CredentialStore creds_store_;
  // In-memory store for list of authorized apps.
  authentication::Db auth_grants_;
  // Flag to track in-memory contents init state.
  unsigned char db_init_option_ = START_INIT;
  // Flag to track the current error status.
  Error error_ = NONE;

  DISALLOW_COPY_AND_ASSIGN(AccountsDbManager);
};

}  // namespace authentication

#endif  // SERVICES_AUTHENTICATION_ACCOUNTS_DB_MANAGER_H_
