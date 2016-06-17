// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/crash/crash_upload.h"

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "base/task_runner_util.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/services/network/interfaces/url_loader.mojom.h"

namespace breakpad {

namespace {

#ifdef NDEBUG
const char kCrashUploadURL[] = "https://clients2.google.com/cr/report";
#else
const char kCrashUploadURL[] = "https://clients2.google.com/cr/staging_report";
#endif
// Minimal delay between two requests to the crash server. This delay is ensured
// by saving a sentinel file each time a request is made.
const int kDelayBetweenCrashesInSeconds = 3600;

// The max size of a multipart/* boundary is 70, but the boundary is prefixed by
// 2 hyphens, and suffixed by an end of line character.
const int kMaxBoundarySize = 73;

struct MinidumpAndBoundary {
  MinidumpAndBoundary() {}
  MinidumpAndBoundary(const base::FilePath& minidump,
                      const std::string& boundary)
      : minidump(minidump), boundary(boundary) {}

  // The path to the minidump file.
  base::FilePath minidump;
  // The boundary for the multipart mime header. This is computed from the
  // content of the minidump.
  std::string boundary;
};

void OnCrashServerResponse(const base::FilePath& minidump,
                           mojo::URLResponsePtr response) {
  base::DeleteFile(minidump, false);
  if (response->status_code == 200) {
    std::string crash_id;
    mojo::common::BlockingCopyToString(response->body.Pass(), &crash_id);
    LOG(INFO) << "Crash uploaded with id: '" << crash_id << "'";
  } else {
    LOG(ERROR) << "Unable to upload crash report. HTTP status: "
               << response->status_code;
  }
}

void OnUploadCrashComplete(const base::FilePath& minidump,
                           scoped_refptr<base::TaskRunner> file_task_runner,
                           mojo::URLLoaderPtr url_loader,
                           mojo::URLResponsePtr response) {
  file_task_runner->PostTask(FROM_HERE,
                             base::Bind(&OnCrashServerResponse, minidump,
                                        base::Passed(response.Pass())));
}

void IgnoreResult(bool result) {
}

// Returns the path to the sentinel. The sentinel is a file whose last modified
// time record the time at which the last request to the crash server was made.
base::FilePath GetFileSentinel(const base::FilePath& dumps_path) {
  return dumps_path.Append("upload.sentinel");
}

void UpdateFileSentinel(const base::FilePath& dumps_path) {
  base::FilePath sentinel = GetFileSentinel(dumps_path);
  int write_return_value = WriteFile(sentinel, nullptr, 0);
  DCHECK_EQ(0, write_return_value);
  bool touch_return_value =
      base::TouchFile(sentinel, base::Time::Now(), base::Time::Now());
  DCHECK(touch_return_value);
}

bool CanUploadCrash(const base::FilePath& dumps_path) {
  base::FilePath sentinel = GetFileSentinel(dumps_path);
  if (!base::PathExists(sentinel))
    return true;
  base::File::Info info;
  if (!GetFileInfo(sentinel, &info)) {
    // Something very wrong happened. Recreate the sentinel and wait for next
    // restart.
    bool return_value = base::DeleteFile(sentinel, true);
    DCHECK(return_value);
    UpdateFileSentinel(dumps_path);
    return false;
  }
  return (base::Time::Now() - info.last_modified) >
         base::TimeDelta::FromSeconds(kDelayBetweenCrashesInSeconds);
}

void UploadCrash(scoped_refptr<base::TaskRunner> file_task_runner,
                 mojo::NetworkServicePtr network_service,
                 const base::FilePath& dumps_path,
                 const MinidumpAndBoundary& minidump_and_boundary) {
  const base::FilePath& minidump = minidump_and_boundary.minidump;
  const std::string& boundary = minidump_and_boundary.boundary;
  if (minidump.empty())
    return;
  mojo::URLRequestPtr request = mojo::URLRequest::New();
  request->url = kCrashUploadURL;
  request->method = "POST";
  auto header = mojo::HttpHeader::New();
  header->name = "Content-Type";
  header->value = "multipart/form-data; boundary=" + boundary;
  request->headers.push_back(header.Pass());
  mojo::DataPipe pipe;
  request->body.push_back(pipe.consumer_handle.Pass());
  mojo::common::CopyFromFile(minidump, pipe.producer_handle.Pass(), 0,
                             file_task_runner.get(), base::Bind(&IgnoreResult));

  mojo::URLLoaderPtr url_loader;
  network_service->CreateURLLoader(GetProxy(&url_loader));
  mojo::URLLoader* loader_ptr = url_loader.get();

  // Update the sentinel before starting the network request. This will prevent
  // any other request to the crash server for one hour.
  UpdateFileSentinel(dumps_path);

  loader_ptr->Start(
      request.Pass(),
      base::Bind(&OnUploadCrashComplete, minidump, file_task_runner,
                 base::Passed(url_loader.Pass())));
}

MinidumpAndBoundary GetMinidumpAndBoundary(const base::FilePath& dumps_path) {
  // Find the last modified minidump. Delete all the other ones.
  base::FilePath minidump;
  base::Time last_modified_time;
  base::FileEnumerator files(dumps_path, false, base::FileEnumerator::FILES,
                             FILE_PATH_LITERAL("*.dmp"));
  for (base::FilePath file = files.Next(); !file.empty(); file = files.Next()) {
    if (minidump.empty()) {
      minidump = file;
      last_modified_time = files.GetInfo().GetLastModifiedTime();
    } else {
      if (files.GetInfo().GetLastModifiedTime() < last_modified_time) {
        base::DeleteFile(file, false);
      } else {
        base::DeleteFile(minidump, false);
        minidump = file;
        last_modified_time = files.GetInfo().GetLastModifiedTime();
      }
    }
  }

  if (minidump.empty() || !CanUploadCrash(dumps_path))
    return MinidumpAndBoundary();
  // Find multipart boundary.
  std::string start_of_file;
  base::ReadFileToString(minidump, &start_of_file, kMaxBoundarySize);
  if (!base::StartsWith(start_of_file, "--", base::CompareCase::SENSITIVE)) {
    LOG(WARNING) << "Corrupted minidump: " << minidump.value();
    base::DeleteFile(minidump, false);
    return MinidumpAndBoundary();
  }
  size_t space_index = start_of_file.find_first_of(base::kWhitespaceASCII);
  if (space_index == std::string::npos || space_index <= 2) {
    // The boundary is either too big, or too short.
    LOG(WARNING) << "Corrupted minidump: " << minidump.value();
    base::DeleteFile(minidump, false);
    return MinidumpAndBoundary();
  }
  return MinidumpAndBoundary(minidump,
                             start_of_file.substr(2, space_index - 2));
}

}  // namespace

void UploadCrashes(const base::FilePath& dumps_path,
                   scoped_refptr<base::TaskRunner> file_task_runner,
                   mojo::NetworkServicePtr network_service) {
  base::PostTaskAndReplyWithResult(
      file_task_runner.get(), FROM_HERE,
      base::Bind(&GetMinidumpAndBoundary, dumps_path),
      base::Bind(&UploadCrash, file_task_runner,
                 base::Passed(network_service.Pass()), dumps_path));
}

}  // namespace breakpad
