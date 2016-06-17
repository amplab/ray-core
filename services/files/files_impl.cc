// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/files/files_impl.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/files/file_path.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/posix/eintr_wrapper.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "mojo/public/cpp/application/connection_context.h"
#include "services/files/directory_impl.h"

namespace mojo {
namespace files {

namespace {

base::ScopedFD CreateAndOpenTemporaryDirectory(
    scoped_ptr<base::ScopedTempDir>* temp_dir) {
  (*temp_dir).reset(new base::ScopedTempDir());
  CHECK((*temp_dir)->CreateUniqueTempDir());

  base::ScopedFD temp_dir_fd(HANDLE_EINTR(
      open((*temp_dir)->path().value().c_str(), O_RDONLY | O_DIRECTORY, 0)));
  PCHECK(temp_dir_fd.is_valid());
  DVLOG(1) << "Made a temporary directory: " << (*temp_dir)->path().value();
  return temp_dir_fd;
}

// Gets the path to a directory relative to the home directory. Returns an empty
// path on failure.
base::FilePath GetHomeRelativePath(const char* dir_name) {
  const char* home_dir_name = getenv("HOME");
  if (!home_dir_name || !home_dir_name[0]) {
    LOG(ERROR) << "HOME not set";
    return base::FilePath();
  }
  return base::FilePath(home_dir_name).Append(dir_name);
}

// Ensures that the parent directory for the (per-application) persistent cache
// directory exists and returns a path to it. Returns an empty path on failure.
base::FilePath EnsureAppPersistentCacheParentDirectory() {
  base::FilePath parent_dir_name =
      GetHomeRelativePath("MojoAppPersistentCaches");

  // Just |mkdir()| it (|mkdir()| isn't interruptible). If it already exists,
  // assume everything is OK (this isn't entirely right -- e.g., it may not be a
  // directory or it may have the wrong permissions -- but it's enough for us to
  // proceed to the next step).
  if (mkdir(parent_dir_name.value().c_str(), S_IRWXU) != 0 && errno != EEXIST) {
    PLOG(ERROR) << "mkdir failed: " << parent_dir_name.value();
    return base::FilePath();
  }
  return parent_dir_name;
}

// Opens the application-specific persistent cache directory for the given |url|
// (creating it if necessary).
base::ScopedFD OpenAppPersistentCacheDirectory(std::string url) {
  base::FilePath parent_dir_name = EnsureAppPersistentCacheParentDirectory();
  if (parent_dir_name.empty())
    return base::ScopedFD();

  // TODO(vtl): We should probably use SHA-256 instead of SHA-1.
  std::string raw_hash = base::SHA1HashString(url);
  std::string hex_hash = base::HexEncode(raw_hash.data(), raw_hash.size());
  base::FilePath dir_name = parent_dir_name.Append(hex_hash);
  if (mkdir(dir_name.value().c_str(), S_IRWXU) != 0 && errno != EEXIST) {
    PLOG(ERROR) << "mkdir failed: " << dir_name.value();
    return base::ScopedFD();
  }

  return base::ScopedFD(
      HANDLE_EINTR(open(dir_name.value().c_str(), O_RDONLY | O_DIRECTORY, 0)));
}

#ifndef NDEBUG
base::ScopedFD OpenMojoDebugDirectory() {
  base::FilePath mojo_debug_dir_name = GetHomeRelativePath("MojoDebug");
  if (mojo_debug_dir_name.empty())
    return base::ScopedFD();
  return base::ScopedFD(HANDLE_EINTR(
      open(mojo_debug_dir_name.value().c_str(), O_RDONLY | O_DIRECTORY, 0)));
}
#endif

}  // namespace

FilesImpl::FilesImpl(const ConnectionContext& connection_context,
                     InterfaceRequest<Files> request)
    : client_url_(connection_context.remote_url),
      binding_(this, request.Pass()) {}

FilesImpl::~FilesImpl() {}

void FilesImpl::OpenFileSystem(const mojo::String& file_system,
                               InterfaceRequest<Directory> directory,
                               const OpenFileSystemCallback& callback) {
  base::ScopedFD dir_fd;
  // Set only if the |DirectoryImpl| will own a temporary directory.
  scoped_ptr<base::ScopedTempDir> temp_dir;
  if (file_system.is_null()) {
    // TODO(vtl): ScopedGeneric (hence ScopedFD) doesn't have an operator=!
    dir_fd.reset(CreateAndOpenTemporaryDirectory(&temp_dir).release());
    DCHECK(temp_dir);
  } else if (file_system.get() == std::string("app_persistent_cache")) {
    // TODO(vtl): ScopedGeneric (hence ScopedFD) doesn't have an operator=!
    dir_fd.reset(OpenAppPersistentCacheDirectory(client_url_).release());
    if (!dir_fd.is_valid()) {
      LOG(ERROR) << "Failed to open app persistent cache directory for "
                 << client_url_;
      callback.Run(Error::UNKNOWN);
      return;
    }
  } else if (file_system.get() == std::string("debug")) {
#ifdef NDEBUG
    LOG(WARNING) << "~/MojoDebug only available in Debug builds";
#else
    // TODO(vtl): ScopedGeneric (hence ScopedFD) doesn't have an operator=!
    dir_fd.reset(OpenMojoDebugDirectory().release());
#endif
    if (!dir_fd.is_valid()) {
      LOG(ERROR) << "~/MojoDebug unavailable";
      callback.Run(Error::UNAVAILABLE);
      return;
    }
  } else {
    LOG(ERROR) << "Unknown file system: " << file_system.get();
    callback.Run(Error::UNIMPLEMENTED);
    return;
  }

  new DirectoryImpl(directory.Pass(), dir_fd.Pass(), temp_dir.Pass());
  callback.Run(Error::OK);
}

}  // namespace files
}  // namespace mojo
