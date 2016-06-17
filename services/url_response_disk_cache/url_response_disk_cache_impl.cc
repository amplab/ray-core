// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/url_response_disk_cache/url_response_disk_cache_impl.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>

#include <type_traits>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "crypto/random.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/public/cpp/bindings/lib/fixed_buffer.h"
#include "mojo/public/interfaces/network/http_header.mojom.h"
#include "services/url_response_disk_cache/url_response_disk_cache_entry.mojom.h"
#include "third_party/zlib/google/zip_reader.h"
#include "url/gurl.h"

namespace mojo {

namespace {

// The current version of the cache. This should only be incremented. When this
// is incremented, all current cache entries will be invalidated.
const uint32_t kCurrentVersion = 1;

// The delay to wait before starting deleting data. This is delayed to not
// interfere with the shell startup.
const uint32_t kTrashDelayInSeconds = 60;

// The delay between the time an entry is invalidated and the cache not
// returning it anymore.
const uint32_t kTimeUntilInvalidationInSeconds = 90;

const char kEtagHeader[] = "etag";

// Create a new identifier for a cache entry. This will be used as an unique
// directory name.
std::string GetNewIdentifier() {
  char bytes[32];
  crypto::RandBytes(bytes, arraysize(bytes));
  return base::HexEncode(bytes, arraysize(bytes));
}

void DoNothing(const base::FilePath& fp1, const base::FilePath& fp2) {}

void MovePathIntoDir(const base::FilePath& source,
                     const base::FilePath& destination) {
  if (!PathExists(source))
    return;

  base::FilePath tmp_dir;
  base::CreateTemporaryDirInDir(destination, "", &tmp_dir);
  base::File::Error error;
  if (!base::ReplaceFile(source, tmp_dir, &error)) {
    LOG(ERROR) << "Failed to clear dir content: " << error;
  }
}

void ClearTrashDir(scoped_refptr<base::TaskRunner> task_runner,
                   const base::FilePath& trash_dir) {
  // Delete the trash directory.
  task_runner->PostDelayedTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(&base::DeleteFile), trash_dir, true),
      base::TimeDelta::FromSeconds(kTrashDelayInSeconds));
}

Array<uint8_t> PathToArray(const base::FilePath& path) {
  if (path.empty())
    return nullptr;
  const std::string& string = path.value();
  auto result = Array<uint8_t>::New(string.size());
  memcpy(&result.front(), string.data(), string.size());
  return result;
}

// This method remove the query string of an url if one is present. It does
// match the behavior of the application manager, which connects to the same app
// if requested twice with different query parameters.
std::string CanonicalizeURL(const std::string& url) {
  GURL gurl(url);

  if (!gurl.has_query()) {
    return gurl.spec();
  }

  GURL::Replacements repl;
  repl.SetQueryStr("");
  std::string result = gurl.ReplaceComponents(repl).spec();
  // Remove the dangling '?' because it's ugly.
  base::ReplaceChars(result, "?", "", &result);
  return result;
}

// This service use a directory under HOME to store all of its data,
base::FilePath GetBaseDirectory() {
  return base::FilePath(getenv("HOME")).Append(".mojo_url_response_disk_cache");
}

// Returns the directory containing live data for the cache.
base::FilePath GetCacheDirectory() {
  return GetBaseDirectory().Append("cache");
}

// Returns a temporary that will be deleted after startup. This is used to have
// a consistent directory for outdated files in case the trash process doesn't
// finish.
base::FilePath GetTrashDirectory() {
  return GetBaseDirectory().Append("trash");
}

// Returns a staging directory to save file before an entry can be saved in the
// database. This directory is deleted when the service is started. This is used
// to prevent leaking files if there is an interruption while downloading a
// response.
base::FilePath GetStagingDirectory() {
  return GetBaseDirectory().Append("staging");
}

// Returns path of the directory that the consumer can use to cache its own
// data.
base::FilePath GetConsumerCacheDirectory(
    const base::FilePath& entry_directory) {
  return entry_directory.Append("consumer_cache");
}

// Returns the path of the sentinel used to mark that a zipped response has
// already been extracted.
base::FilePath GetExtractionSentinel(const base::FilePath& entry_directory) {
  return entry_directory.Append("extraction_sentinel");
}

// Returns the path of the directory where a zipped content is extracted.
base::FilePath GetExtractionDirectory(const base::FilePath& entry_directory) {
  return entry_directory.Append("extracted");
}

// Runs the given callback. If |success| is false, call back with an error.
// Otherwise, store a new entry in the databse, then call back with the content
// path and the consumer cache path.
void RunCallbackWithSuccess(
    const URLResponseDiskCacheImpl::ResponseFileAndCacheDirCallback& callback,
    const std::string& identifier,
    const std::string& request_origin,
    const std::string& url,
    URLResponsePtr response,
    scoped_refptr<URLResponseDiskCacheDB> db,
    scoped_refptr<base::TaskRunner> task_runner,
    bool success) {
  TRACE_EVENT2("url_response_disk_cache", "RunCallbackWithSuccess", "url", url,
               "identifier", identifier);
  if (!success) {
    callback.Run(base::FilePath(), base::FilePath());
    return;
  }

  base::FilePath staged_response_body_path =
      GetStagingDirectory().Append(identifier);
  base::FilePath entry_directory = GetCacheDirectory().Append(identifier);
  base::FilePath response_body_path = entry_directory.Append(identifier);
  base::FilePath consumer_cache_directory =
      GetConsumerCacheDirectory(entry_directory);

  CacheEntryPtr entry = CacheEntry::New();
  entry->response = response.Pass();
  entry->entry_directory = entry_directory.value();
  entry->response_body_path = response_body_path.value();
  entry->last_invalidation = base::Time::Max().ToInternalValue();

  db->PutNew(request_origin, url, entry.Pass());

  if (!base::CreateDirectoryAndGetError(entry_directory, nullptr) ||
      !base::CreateDirectoryAndGetError(consumer_cache_directory, nullptr) ||
      !base::ReplaceFile(staged_response_body_path, response_body_path,
                         nullptr)) {
    MovePathIntoDir(entry_directory, GetStagingDirectory());
    callback.Run(base::FilePath(), base::FilePath());
    return;
  }

  callback.Run(response_body_path, consumer_cache_directory);
}

// Run the given mojo callback with the given paths.
void RunMojoCallback(
    const Callback<void(Array<uint8_t>, Array<uint8_t>)>& callback,
    const base::FilePath& path1,
    const base::FilePath& path2) {
  callback.Run(PathToArray(path1), PathToArray(path2));
}

mojo::URLResponsePtr GetMinimalResponse(const std::string& url) {
  mojo::URLResponsePtr response = mojo::URLResponse::New();
  response->url = url;
  response->status_code = 200;
  return response;
}

void RunMojoCallbackWithResponse(
    const Callback<void(mojo::URLResponsePtr, Array<uint8_t>, Array<uint8_t>)>&
        callback,
    const std::string& url,
    const base::FilePath& path1,
    const base::FilePath& path2) {
  mojo::URLResponsePtr response;
  if (!path1.empty())
    response = GetMinimalResponse(url);
  callback.Run(response.Pass(), PathToArray(path1), PathToArray(path2));
}

// Returns the list of values for the given |header_name| in the given list of
// headers.
std::vector<std::string> GetHeaderValues(const std::string& header_name,
                                         const Array<HttpHeaderPtr>& headers) {
  std::vector<std::string> result;
  for (size_t i = 0u; i < headers.size(); ++i) {
    const std::string& name = headers[i]->name.get();
    if (base::LowerCaseEqualsASCII(name, header_name.c_str()))
      result.push_back(headers[i]->value);
  }
  return result;
}

// Returns whether the given |entry| is valid.
bool IsCacheEntryValid(const CacheEntryPtr& entry) {
  return entry &&
         PathExists(base::FilePath::FromUTF8Unsafe(entry->response_body_path));
}

// Returns whether the given directory |entry| is valid and its content can be
// used for the given |response|.
bool IsCacheEntryFresh(const URLResponsePtr& response,
                       const CacheEntryPtr& entry) {
  DCHECK(response);
  if (!IsCacheEntryValid(entry))
    return false;

  // If |entry| or |response| has not headers, it is not possible to check if
  // the entry is valid, so returns |false|.
  if (entry->response->headers.is_null() || response->headers.is_null())
    return false;

  // Only handle etag for the moment.
  std::string etag_header_name = kEtagHeader;
  std::vector<std::string> entry_etags =
      GetHeaderValues(etag_header_name, entry->response->headers);
  if (entry_etags.size() == 0)
    return false;
  std::vector<std::string> response_etags =
      GetHeaderValues(etag_header_name, response->headers);
  if (response_etags.size() == 0)
    return false;

  // Looking for the first etag header.
  return entry_etags[0] == response_etags[0];
}

void UpdateLastInvalidation(scoped_refptr<URLResponseDiskCacheDB> db,
                            CacheKeyPtr key,
                            const base::Time& time) {
  CacheEntryPtr entry = db->Get(key.Clone());
  entry->last_invalidation = time.ToInternalValue();
  db->Put(key.Pass(), entry.Pass());
}

void PruneCache(scoped_refptr<URLResponseDiskCacheDB> db,
                const scoped_ptr<URLResponseDiskCacheDB::Iterator>& iterator) {
  CacheKeyPtr last_key;
  CacheKeyPtr key;
  CacheEntryPtr entry;
  while (iterator->HasNext()) {
    iterator->GetNext(&key, &entry);
    if (last_key && last_key->request_origin == key->request_origin &&
        last_key->url == key->url) {
      base::FilePath entry_directory =
          base::FilePath::FromUTF8Unsafe(entry->entry_directory);
      if (base::DeleteFile(entry_directory, true))
        db->Delete(key.Clone());
    }
    last_key = key.Pass();
  }
}

}  // namespace

// static
scoped_refptr<URLResponseDiskCacheDB> URLResponseDiskCacheImpl::CreateDB(
    scoped_refptr<base::TaskRunner> task_runner,
    bool force_clean) {
  // Create the trash directory if needed.
  base::FilePath trash_dir = GetTrashDirectory();
  base::CreateDirectory(trash_dir);

  // Clean the trash directory when exiting this method.
  base::ScopedClosureRunner trash_cleanup(
      base::Bind(&ClearTrashDir, task_runner, trash_dir));

  // Move the staging directory to trash.
  MovePathIntoDir(GetStagingDirectory(), trash_dir);
  // And recreate it.
  base::CreateDirectory(GetStagingDirectory());

  base::FilePath db_path = GetBaseDirectory().Append("db");

  if (!force_clean && PathExists(db_path)) {
    scoped_refptr<URLResponseDiskCacheDB> db =
        new URLResponseDiskCacheDB(db_path);
    if (db->GetVersion() == kCurrentVersion) {
      task_runner->PostDelayedTask(
          FROM_HERE,
          base::Bind(&PruneCache, db, base::Passed(db->GetIterator())),
          base::TimeDelta::FromSeconds(kTrashDelayInSeconds));
      return db;
    }
  }

  // Move the database to trash.
  MovePathIntoDir(db_path, trash_dir);
  // Move the current cache content to trash.
  MovePathIntoDir(GetCacheDirectory(), trash_dir);

  scoped_refptr<URLResponseDiskCacheDB> result =
      new URLResponseDiskCacheDB(db_path);
  result->SetVersion(kCurrentVersion);
  return result;
}

URLResponseDiskCacheImpl::URLResponseDiskCacheImpl(
    scoped_refptr<base::TaskRunner> task_runner,
    URLResponseDiskCacheDelegate* delegate,
    scoped_refptr<URLResponseDiskCacheDB> db,
    const std::string& remote_application_url,
    InterfaceRequest<URLResponseDiskCache> request)
    : task_runner_(task_runner),
      delegate_(delegate),
      db_(db),
      binding_(this, request.Pass()) {
  request_origin_ = GURL(remote_application_url).GetOrigin().spec();
}

URLResponseDiskCacheImpl::~URLResponseDiskCacheImpl() {
}

bool IsInvalidated(const CacheEntryPtr& entry) {
  if (!entry)
    return true;
  return base::Time::Now() -
             base::Time::FromInternalValue(entry->last_invalidation) >
         base::TimeDelta::FromSeconds(kTimeUntilInvalidationInSeconds);
}

void URLResponseDiskCacheImpl::Get(const String& url,
                                   const GetCallback& callback) {
  std::string canonilized_url = CanonicalizeURL(url);
  CacheKeyPtr key;
  CacheEntryPtr entry = db_->GetNewest(request_origin_, canonilized_url, &key);
  if (!entry && delegate_) {
    // This is the first request for the given |url|. Ask |delegate| for initial
    // content.

    std::string identifier = GetNewIdentifier();
    // The content is copied to the staging directory so that files are not
    // leaked if the shell terminates before an entry is saved to the database.
    base::FilePath staged_response_body_path =
        GetStagingDirectory().Append(identifier);

    delegate_->GetInitialPath(
        task_runner_, canonilized_url, staged_response_body_path,
        base::Bind(
            RunCallbackWithSuccess,
            base::Bind(&RunMojoCallbackWithResponse, callback, canonilized_url),
            identifier, request_origin_, canonilized_url,
            base::Passed(GetMinimalResponse(canonilized_url)), db_,
            task_runner_));
    return;
  }
  if (IsInvalidated(entry) || !IsCacheEntryValid(entry)) {
    callback.Run(URLResponsePtr(), nullptr, nullptr);
    return;
  }
  callback.Run(
      entry->response.Pass(),
      PathToArray(base::FilePath::FromUTF8Unsafe(entry->response_body_path)),
      PathToArray(GetConsumerCacheDirectory(
          base::FilePath::FromUTF8Unsafe(entry->entry_directory))));
  UpdateLastInvalidation(db_, key.Pass(), base::Time::Now());
}

void URLResponseDiskCacheImpl::Validate(const String& url) {
  CacheKeyPtr key;
  CacheEntryPtr entry =
      db_->GetNewest(request_origin_, CanonicalizeURL(url), &key);
  if (entry)
    UpdateLastInvalidation(db_, key.Pass(), base::Time::Max());
}

void URLResponseDiskCacheImpl::Update(URLResponsePtr response) {
  UpdateAndGetInternal(response.Pass(), base::Bind(&DoNothing));
}

void URLResponseDiskCacheImpl::UpdateAndGet(
    URLResponsePtr response,
    const UpdateAndGetCallback& callback) {
  UpdateAndGetInternal(response.Pass(), base::Bind(&RunMojoCallback, callback));
}

void URLResponseDiskCacheImpl::UpdateAndGetExtracted(
    URLResponsePtr response,
    const UpdateAndGetExtractedCallback& callback) {
  UpdateAndGetInternal(
      response.Pass(),
      base::Bind(&URLResponseDiskCacheImpl::UpdateAndGetExtractedInternal,
                 base::Unretained(this),
                 base::Bind(&RunMojoCallback, callback)));
}

void URLResponseDiskCacheImpl::UpdateAndGetInternal(
    URLResponsePtr response,
    const ResponseFileAndCacheDirCallback& callback) {
  if (response->error ||
      (response->status_code >= 400 && response->status_code < 600)) {
    callback.Run(base::FilePath(), base::FilePath());
    return;
  }

  std::string url = CanonicalizeURL(response->url);

  // Check if the response is cached and valid. If that's the case, returns
  // the cached value.
  CacheKeyPtr key;
  CacheEntryPtr entry = db_->GetNewest(request_origin_, url, &key);
  if (IsCacheEntryFresh(response, entry)) {
    callback.Run(base::FilePath::FromUTF8Unsafe(entry->response_body_path),
                 GetConsumerCacheDirectory(
                     base::FilePath::FromUTF8Unsafe(entry->entry_directory)));
    UpdateLastInvalidation(db_, key.Pass(), base::Time::Max());
    return;
  }

  if (!response->body.is_valid()) {
    callback.Run(base::FilePath(), base::FilePath());
    return;
  }

  std::string identifier = GetNewIdentifier();
  // The content is copied to the staging directory so that files are not leaked
  // if the shell terminates before an entry is saved to the database.
  base::FilePath staged_response_body_path =
      GetStagingDirectory().Append(identifier);

  ScopedDataPipeConsumerHandle body = response->body.Pass();

  // Asynchronously copy the response body to the staging directory. The
  // callback will move it to the cache directory and save an entry in the
  // database only if the copy of the body succeded.
  common::CopyToFile(
      body.Pass(), staged_response_body_path, task_runner_.get(),
      base::Bind(&RunCallbackWithSuccess, callback, identifier, request_origin_,
                 url, base::Passed(response.Pass()), db_, task_runner_));
}

void URLResponseDiskCacheImpl::UpdateAndGetExtractedInternal(
    const ResponseFileAndCacheDirCallback& callback,
    const base::FilePath& response_body_path,
    const base::FilePath& consumer_cache_directory) {
  TRACE_EVENT1("url_response_disk_cache", "UpdateAndGetExtractedInternal",
               "response_body_path", response_body_path.value());
  // If it is not possible to get the cached file, returns an error.
  if (response_body_path.empty()) {
    callback.Run(base::FilePath(), base::FilePath());
    return;
  }

  base::FilePath entry_directory = consumer_cache_directory.DirName();
  base::FilePath extraction_directory = GetExtractionDirectory(entry_directory);
  base::FilePath extraction_sentinel = GetExtractionSentinel(entry_directory);

  if (PathExists(extraction_sentinel)) {
    callback.Run(extraction_directory, consumer_cache_directory);
    return;
  }

  if (PathExists(extraction_directory)) {
    if (!base::DeleteFile(extraction_directory, true)) {
      callback.Run(base::FilePath(), base::FilePath());
      return;
    }
  }

  // Unzip the content to the extracted directory. In case of any error, returns
  // an error.
  zip::ZipReader reader;
  if (!reader.Open(response_body_path)) {
    callback.Run(base::FilePath(), base::FilePath());
    return;
  }
  while (reader.HasMore()) {
    bool success = reader.OpenCurrentEntryInZip();
    success = success &&
              reader.ExtractCurrentEntryIntoDirectory(extraction_directory);
    success = success && reader.AdvanceToNextEntry();
    if (!success) {
      callback.Run(base::FilePath(), base::FilePath());
      return;
    }
  }
  // We can ignore write error, as it will just force to clear the cache on the
  // next request.
  WriteFile(GetExtractionSentinel(entry_directory), nullptr, 0);
  callback.Run(extraction_directory, consumer_cache_directory);
}

}  // namespace mojo
