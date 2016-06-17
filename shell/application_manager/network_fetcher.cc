// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/application_manager/network_fetcher.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/process/process.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "crypto/secure_hash.h"
#include "crypto/sha2.h"
#include "mojo/converters/url/url_type_converters.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"

namespace shell {

namespace {
#if defined(OS_LINUX)
char kArchitecture[] = "linux-x64";
#elif defined(OS_ANDROID)
char kArchitecture[] = "android-arm";
#elif defined(OS_MACOSX)
char kArchitecture[] = "macosx";
#else
#error "Unsupported."
#endif

const bool kScheduleUpdate = true;
const bool kDoNotScheduleUpdate = false;

// The delay to wait before trying to update an application after having served
// it from the cache.
const uint32_t kUpdateApplicationDelayInSeconds = 60;

base::FilePath ToFilePath(const mojo::Array<uint8_t>& array) {
  return base::FilePath(
      std::string(reinterpret_cast<const char*>(&array.front()), array.size()));
}

void IgnoreResult(bool result) {}

mojo::URLRequestPtr GetRequest(const GURL& url, bool disable_cache) {
  mojo::URLRequestPtr request(mojo::URLRequest::New());
  request->url = mojo::String::From(url);
  request->auto_follow_redirects = false;
  if (disable_cache)
    request->cache_mode = mojo::URLRequest::URLRequest::CacheMode::BYPASS_CACHE;
  auto architecture_header = mojo::HttpHeader::New();
  architecture_header->name = "X-Architecture";
  architecture_header->value = kArchitecture;
  mojo::Array<mojo::HttpHeaderPtr> headers;
  headers.push_back(architecture_header.Pass());
  request->headers = headers.Pass();

  return request;
}

// Clone an URLResponse, except for the body.
mojo::URLResponsePtr CloneResponse(const mojo::URLResponsePtr& response) {
  mojo::URLResponsePtr cloned = mojo::URLResponse::New();
  cloned->error = response->error.Clone();
  cloned->url = response->url;
  cloned->status_code = response->status_code;
  cloned->status_line = response->status_line;
  cloned->headers = response->headers.Clone();
  cloned->mime_type = response->mime_type;
  cloned->charset = response->charset;
  cloned->redirect_method = response->redirect_method;
  cloned->redirect_url = response->redirect_url;
  cloned->redirect_referrer = response->redirect_referrer;
  return cloned;
}

// This class is self owned and will delete itself after having tried to update
// the application cache.
class ApplicationUpdater : public base::MessageLoop::DestructionObserver {
 public:
  ApplicationUpdater(const GURL& url,
                     const base::TimeDelta& update_delay,
                     mojo::URLResponseDiskCache* url_response_disk_cache,
                     mojo::NetworkService* network_service);
  ~ApplicationUpdater() override;

 private:
  // DestructionObserver
  void WillDestroyCurrentMessageLoop() override;

  void UpdateApplication();
  void OnLoadComplete(mojo::URLResponsePtr response);

  GURL url_;
  base::TimeDelta update_delay_;
  mojo::URLResponseDiskCache* url_response_disk_cache_;
  mojo::NetworkService* network_service_;
  mojo::URLLoaderPtr url_loader_;
};

ApplicationUpdater::ApplicationUpdater(
    const GURL& url,
    const base::TimeDelta& update_delay,
    mojo::URLResponseDiskCache* url_response_disk_cache,
    mojo::NetworkService* network_service)
    : url_(url),
      update_delay_(update_delay),
      url_response_disk_cache_(url_response_disk_cache),
      network_service_(network_service) {
  base::MessageLoop::current()->AddDestructionObserver(this);
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE, base::Bind(&ApplicationUpdater::UpdateApplication,
                            base::Unretained(this)),
      update_delay_);
}

ApplicationUpdater::~ApplicationUpdater() {
  base::MessageLoop::current()->RemoveDestructionObserver(this);
}

void ApplicationUpdater::WillDestroyCurrentMessageLoop() {
  delete this;
}

void ApplicationUpdater::UpdateApplication() {
  network_service_->CreateURLLoader(GetProxy(&url_loader_));
  url_loader_->Start(
      GetRequest(url_, false),
      base::Bind(&ApplicationUpdater::OnLoadComplete, base::Unretained(this)));
}

void ApplicationUpdater::OnLoadComplete(mojo::URLResponsePtr response) {
  std::string url = response->url;
  url_response_disk_cache_->Update(response.Pass());
  url_response_disk_cache_->Validate(url);
  delete this;
}

}  // namespace

NetworkFetcher::NetworkFetcher(
    bool disable_cache,
    bool force_offline_by_default,
    const GURL& url,
    mojo::URLResponseDiskCache* url_response_disk_cache,
    mojo::NetworkService* network_service,
    const FetchCallback& loader_callback)
    : Fetcher(loader_callback),
      disable_cache_(disable_cache),
      force_offline_by_default_(force_offline_by_default),
      url_(url),
      url_response_disk_cache_(url_response_disk_cache),
      network_service_(network_service),
      weak_ptr_factory_(this) {
  if (CanLoadDirectlyFromCache()) {
    LoadFromCache();
  } else {
    StartNetworkRequest();
  }
}

NetworkFetcher::~NetworkFetcher() {
}

const GURL& NetworkFetcher::GetURL() const {
  return url_;
}

GURL NetworkFetcher::GetRedirectURL() const {
  if (!response_)
    return GURL::EmptyGURL();

  if (response_->redirect_url.is_null())
    return GURL::EmptyGURL();

  return GURL(response_->redirect_url);
}

mojo::URLResponsePtr NetworkFetcher::AsURLResponse(
    base::TaskRunner* task_runner,
    uint32_t skip) {
  DCHECK(response_);
  DCHECK(!path_.empty());
  mojo::DataPipe data_pipe;
  response_->body = data_pipe.consumer_handle.Pass();
  mojo::common::CopyFromFile(path_, data_pipe.producer_handle.Pass(), skip,
                             task_runner, base::Bind(&IgnoreResult));
  return response_.Pass();
}

void NetworkFetcher::AsPath(
    base::TaskRunner* task_runner,
    base::Callback<void(const base::FilePath&, bool)> callback) {
  // This should only called once, when we have a response.
  DCHECK(response_.get());

  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, path_, base::PathExists(path_)));
  response_.reset();
  return;
}

std::string NetworkFetcher::MimeType() {
  return response_->mime_type;
}

bool NetworkFetcher::HasMojoMagic() {
  return Fetcher::HasMojoMagic(path_);
}

bool NetworkFetcher::PeekFirstLine(std::string* line) {
  return Fetcher::PeekFirstLine(path_, line);
}

bool NetworkFetcher::CanLoadDirectlyFromCache() {
  if (disable_cache_)
    return false;

  if (force_offline_by_default_)
    return true;

  const std::string& host = url_.host();
  return !(host == "localhost" || host == "127.0.0.1" || host == "[::1]");
}

void NetworkFetcher::LoadFromCache() {
  url_response_disk_cache_->Get(
      mojo::String::From(url_),
      base::Bind(&NetworkFetcher::OnResponseReceived, base::Unretained(this),
                 kScheduleUpdate));
}

void NetworkFetcher::OnResponseReceived(bool schedule_update,
                                        mojo::URLResponsePtr response,
                                        mojo::Array<uint8_t> path_as_array,
                                        mojo::Array<uint8_t> cache_dir) {
  if (!response) {
    // Not in cache, loading from net.
    StartNetworkRequest();
    return;
  }
  if (schedule_update) {
    // The response has been found in the cache. Plan updating the application.
    new ApplicationUpdater(
        url_, base::TimeDelta::FromSeconds(kUpdateApplicationDelayInSeconds),
        url_response_disk_cache_, network_service_);
  }
  response_ = response.Pass();
  path_ = ToFilePath(path_as_array);
  RecordCacheToURLMapping(path_, url_);
  loader_callback_.Run(make_scoped_ptr(this));
}

void NetworkFetcher::StartNetworkRequest() {
  TRACE_EVENT_ASYNC_BEGIN1("mojo_shell", "NetworkFetcher::NetworkRequest", this,
                           "url", url_.spec());
  network_service_->CreateURLLoader(GetProxy(&url_loader_));
  url_loader_->Start(GetRequest(url_, disable_cache_),
                     base::Bind(&NetworkFetcher::OnLoadComplete,
                                weak_ptr_factory_.GetWeakPtr()));
}

void NetworkFetcher::OnLoadComplete(mojo::URLResponsePtr response) {
  TRACE_EVENT_ASYNC_END0("mojo_shell", "NetworkFetcher::NetworkRequest", this);
  if (response->error) {
    LOG(ERROR) << "Error (" << response->error->code << ": "
               << response->error->description << ") while fetching "
               << response->url;
    loader_callback_.Run(nullptr);
    delete this;
    return;
  }

  if (response->status_code >= 400 && response->status_code < 600) {
    LOG(ERROR) << "Error (" << response->status_code << ": "
               << response->status_line << "): "
               << "while fetching " << response->url;
    loader_callback_.Run(nullptr);
    delete this;
    return;
  }

  if (!response->redirect_url.is_null()) {
    response_ = response.Pass();
    loader_callback_.Run(make_scoped_ptr(this));
    return;
  }

  mojo::URLResponsePtr cloned_response = CloneResponse(response);
  url_response_disk_cache_->UpdateAndGet(
      response.Pass(), base::Bind(&NetworkFetcher::OnFileSavedToCache,
                                  weak_ptr_factory_.GetWeakPtr(),
                                  base::Passed(cloned_response.Pass())));
}

void NetworkFetcher::OnFileSavedToCache(mojo::URLResponsePtr response,
                                        mojo::Array<uint8_t> path_as_array,
                                        mojo::Array<uint8_t> cache_dir) {
  if (!path_as_array) {
    LOG(WARNING) << "Error when retrieving content from cache for: "
                 << url_.spec();
    loader_callback_.Run(nullptr);
    delete this;
    return;
  }
  OnResponseReceived(kDoNotScheduleUpdate, response.Pass(),
                     path_as_array.Pass(), cache_dir.Pass());
}

void NetworkFetcher::RecordCacheToURLMapping(const base::FilePath& path,
                                             const GURL& url) {
  // This is used to extract symbols on android.
  // TODO(eseidel): All users of this log should move to using the map file.
  LOG(INFO) << "Caching mojo app " << url << " at " << path.value();

  base::FilePath temp_dir;
  base::GetTempDir(&temp_dir);
  base::ProcessId pid = base::Process::Current().Pid();
  std::string map_name = base::StringPrintf("mojo_shell.%d.maps", pid);
  base::FilePath map_path = temp_dir.Append(map_name);

  // TODO(eseidel): Paths or URLs with spaces will need quoting.
  std::string map_entry =
      base::StringPrintf("%s %s\n", path.value().c_str(), url.spec().c_str());
  // TODO(eseidel): AppendToFile is missing O_CREAT, crbug.com/450696
  if (!PathExists(map_path)) {
    base::WriteFile(map_path, map_entry.data(), map_entry.length());
  } else {
    base::AppendToFile(map_path, map_entry.data(), map_entry.length());
  }
}

}  // namespace shell
