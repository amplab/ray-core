// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/log/log_impl.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "mojo/public/cpp/application/connection_context.h"
#include "mojo/services/log/interfaces/entry.mojom.h"

namespace mojo {
namespace log {
namespace {

std::string LogLevelToString(int32_t log_level) {
  if (log_level <= kLogLevelVerbose - 3)
    return "VERBOSE4+";
  switch (log_level) {
    case kLogLevelVerbose - 2:
      return "VERBOSE3";
    case kLogLevelVerbose - 1:
      return "VERBOSE2";
    case kLogLevelVerbose:
      return "VERBOSE1";
    case kLogLevelInfo:
      return "INFO";
    case kLogLevelWarning:
      return "WARNING";
    case kLogLevelError:
      return "ERROR";
  }
  return "FATAL";
}

}  // namespace

LogImpl::LogImpl(const std::string& remote_url,
                 InterfaceRequest<Log> request,
                 PrintLogMessageFunction print_log_message_function)
    : remote_url_(remote_url),
      binding_(this, std::move(request)),
      print_log_message_function_(print_log_message_function) {}

LogImpl::~LogImpl() {}

// static
void LogImpl::Create(const ConnectionContext& connection_context,
                     InterfaceRequest<Log> request,
                     PrintLogMessageFunction print_log_message_function) {
  DCHECK(print_log_message_function);

  const std::string& remote_url = connection_context.remote_url;
  if (remote_url.empty()) {
    LOG(ERROR) << "No remote URL.";
    return;
  }

  new LogImpl(remote_url, std::move(request),
              std::move(print_log_message_function));
}

void LogImpl::AddEntry(EntryPtr entry) {
  DCHECK(entry);
  print_log_message_function_(FormatEntry(entry));
}

// This should return:
// <REMOTE_URL> [LOG_LEVEL] SOURCE_FILE:SOURCE_LINE MESSAGE
std::string LogImpl::FormatEntry(const EntryPtr& entry) {
  std::string source;
  if (entry->source_file) {
    source += entry->source_file.To<std::string>();
    if (entry->source_line) {
      base::StringAppendF(&source, ":%u", entry->source_line);
    }
    source += ": ";
  }

  return base::StringPrintf(
      "<%s> [%s] %s%s", remote_url_.c_str(),
      LogLevelToString(entry->log_level).c_str(), source.c_str(),
      entry->message ? entry->message.To<std::string>().c_str()
                     : "<no message>");
}

}  // namespace log
}  // namespace mojo
