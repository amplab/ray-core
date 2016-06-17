// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/application_manager/local_fetcher.h"

#include <sys/stat.h>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "mojo/converters/url/url_type_converters.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "url/url_util.h"

namespace shell {

namespace {

void IgnoreResult(bool result) {
}

}  // namespace

// A loader for local files.
LocalFetcher::LocalFetcher(const GURL& url,
                           const GURL& url_without_query,
                           const FetchCallback& loader_callback)
    : Fetcher(loader_callback), url_(url), path_(UrlToFile(url_without_query)) {
  TRACE_EVENT1("mojo_shell", "LocalFetcher::LocalFetcher", "url", url.spec());
  loader_callback_.Run(make_scoped_ptr(this));
}

base::FilePath LocalFetcher::UrlToFile(const GURL& url) {
  DCHECK(url.SchemeIsFile());
  url::RawCanonOutputW<1024> output;
  url::DecodeURLEscapeSequences(url.path().data(),
                                static_cast<int>(url.path().length()), &output);
  base::string16 decoded_path = base::string16(output.data(), output.length());
  return base::FilePath(base::UTF16ToUTF8(decoded_path));
}

const GURL& LocalFetcher::GetURL() const {
  return url_;
}

GURL LocalFetcher::GetRedirectURL() const {
  return GURL::EmptyGURL();
}

mojo::URLResponsePtr LocalFetcher::AsURLResponse(base::TaskRunner* task_runner,
                                                 uint32_t skip) {
  mojo::URLResponsePtr response(mojo::URLResponse::New());
  response->url = mojo::String::From(url_);
  mojo::DataPipe data_pipe;
  response->body = data_pipe.consumer_handle.Pass();
  base::stat_wrapper_t stat_result;
#if defined(OS_MACOSX)
  if (stat(path_.value().c_str(), &stat_result) == 0) {
#else
  if (stat64(path_.value().c_str(), &stat_result) == 0) {
#endif
    auto content_length_header = mojo::HttpHeader::New();
    content_length_header->name = "Content-Length";
    content_length_header->value =
        base::StringPrintf("%" PRId64, stat_result.st_size);
    response->headers.push_back(content_length_header.Pass());
    auto etag_header = mojo::HttpHeader::New();
    etag_header->name = "ETag";
    etag_header->value = base::StringPrintf(
        "\"%" PRId64 "-%" PRId64 "-%" PRId64 "\"",
        static_cast<uint64_t>(stat_result.st_dev), stat_result.st_ino,
        static_cast<uint64_t>(stat_result.st_mtime));
    response->headers.push_back(etag_header.Pass());
  }
  mojo::common::CopyFromFile(path_, data_pipe.producer_handle.Pass(), skip,
                             task_runner, base::Bind(&IgnoreResult));
  return response;
}

void LocalFetcher::AsPath(
    base::TaskRunner* task_runner,
    base::Callback<void(const base::FilePath&, bool)> callback) {
  // Async for consistency with network case.
  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(callback, path_, base::PathExists(path_)));
}

std::string LocalFetcher::MimeType() {
  return "";
}

bool LocalFetcher::HasMojoMagic() {
  return Fetcher::HasMojoMagic(path_);
}

bool LocalFetcher::PeekFirstLine(std::string* line) {
  return Fetcher::PeekFirstLine(path_, line);
}

}  // namespace shell
