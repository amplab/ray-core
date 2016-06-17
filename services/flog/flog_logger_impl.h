// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_FLOG_FLOG_LOGGER_IMPL_H_
#define MOJO_SERVICES_FLOG_FLOG_LOGGER_IMPL_H_

#include <memory>
#include <vector>

#include "mojo/public/cpp/bindings/binding.h"
#include "services/flog/flog_directory.h"
#include "services/flog/flog_service_impl.h"

namespace mojo {
namespace flog {

// FlogLogger implementation.
class FlogLoggerImpl : public FlogServiceImpl::ProductBase,
                       MessageReceiverWithResponderStatus {
 public:
  static std::shared_ptr<FlogLoggerImpl> Create(
      InterfaceRequest<FlogLogger> request,
      uint32_t log_id,
      const std::string& label,
      std::shared_ptr<FlogDirectory> directory,
      FlogServiceImpl* owner);

  ~FlogLoggerImpl() override;

  // MessageReceiverWithResponderStatus implementation.
  bool Accept(Message* message) override;

  bool AcceptWithResponder(Message* message,
                           MessageReceiverWithStatus* responder) override;

 private:
  FlogLoggerImpl(InterfaceRequest<FlogLogger> request,
                 uint32_t log_id,
                 const std::string& label,
                 std::shared_ptr<FlogDirectory> directory,
                 FlogServiceImpl* owner);

  void WriteData(uint32_t data_size, const void* data);

  std::unique_ptr<mojo::internal::Router> router_;
  files::FilePtr file_;
};

}  // namespace flog
}  // namespace mojo

#endif  // MOJO_SERVICES_FLOG_FLOG_LOGGER_IMPL_H_
