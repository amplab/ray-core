// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_LOG_LOG_IMPL_H_
#define SERVICES_LOG_LOG_IMPL_H_

#include <functional>
#include <string>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/log/interfaces/entry.mojom.h"
#include "mojo/services/log/interfaces/log.mojom.h"

namespace mojo {

struct ConnectionContext;

namespace log {

// This is an implementation of the log service
// (see mojo/services/log/interfaces/log.mojom). It formats incoming messages
// and "prints" them using a supplied function.
//
// This service implementation binds a new Log implementation for each incoming
// application connection.
class LogImpl : public Log {
 public:
  // Function that prints the given (fully-formatted) log message.
  using PrintLogMessageFunction =
      std::function<void(const std::string& message)>;

  // Note that |print_log_message_function| may be called many times, for the
  // lifetime of the created object.
  static void Create(const ConnectionContext& connection_context,
                     InterfaceRequest<Log> request,
                     PrintLogMessageFunction print_log_message_function);

  // |Log| implementation:
  void AddEntry(EntryPtr entry) override;

 private:
  LogImpl(const std::string& remote_url,
          InterfaceRequest<Log> request,
          PrintLogMessageFunction print_log_message_function);
  ~LogImpl() override;

  std::string FormatEntry(const EntryPtr& entry);

  const std::string remote_url_;
  StrongBinding<Log> binding_;
  const PrintLogMessageFunction print_log_message_function_;

  DISALLOW_COPY_AND_ASSIGN(LogImpl);
};

}  // namespace log
}  // namespace mojo

#endif  // SERVICES_LOG_LOG_IMPL_H_
