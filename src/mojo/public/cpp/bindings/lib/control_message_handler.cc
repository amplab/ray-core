// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/control_message_handler.h"

#include "mojo/public/cpp/bindings/lib/message_builder.h"
#include "mojo/public/cpp/environment/logging.h"
#include "mojo/public/interfaces/bindings/interface_control_messages.mojom.h"

namespace mojo {
namespace internal {

// static
bool ControlMessageHandler::IsControlMessage(const Message* message) {
  return !!(message->header()->name & 0x80000000);
}

ControlMessageHandler::ControlMessageHandler(uint32_t interface_version)
    : interface_version_(interface_version) {
}

ControlMessageHandler::~ControlMessageHandler() {
}

bool ControlMessageHandler::Accept(Message* message) {
  if (message->header()->name == kRunOrClosePipeMessageId)
    return RunOrClosePipe(message);

  MOJO_DCHECK(false) << "Bad control message (no response): name = "
                     << message->header()->name;
  return false;
}

bool ControlMessageHandler::AcceptWithResponder(
    Message* message,
    MessageReceiverWithStatus* responder) {
  if (message->header()->name == kRunMessageId)
    return Run(message, responder);

  MOJO_DCHECK(false) << "Bad control message (with response): name = "
                     << message->header()->name;
  return false;
}

bool ControlMessageHandler::Run(Message* message,
                                MessageReceiverWithStatus* responder) {
  RunResponseMessageParamsPtr response_params_ptr(
      RunResponseMessageParams::New());
  response_params_ptr->reserved0 = 16u;
  response_params_ptr->reserved1 = 0u;
  response_params_ptr->query_version_result = QueryVersionResult::New();
  response_params_ptr->query_version_result->version = interface_version_;

  size_t size = GetSerializedSize_(*response_params_ptr);
  ResponseMessageBuilder builder(kRunMessageId, size, message->request_id());

  RunResponseMessageParams_Data* response_params = nullptr;
  auto result =
      Serialize_(response_params_ptr.get(), builder.buffer(), &response_params);
  MOJO_DCHECK(result == ValidationError::NONE);

  response_params->EncodePointersAndHandles(
      builder.message()->mutable_handles());
  bool ok = responder->Accept(builder.message());
  MOJO_ALLOW_UNUSED_LOCAL(ok);
  delete responder;

  return true;
}

bool ControlMessageHandler::RunOrClosePipe(Message* message) {
  RunOrClosePipeMessageParams_Data* params =
      reinterpret_cast<RunOrClosePipeMessageParams_Data*>(
          message->mutable_payload());
  params->DecodePointersAndHandles(message->mutable_handles());

  RunOrClosePipeMessageParamsPtr params_ptr(RunOrClosePipeMessageParams::New());
  Deserialize_(params, params_ptr.get());

  return interface_version_ >= params_ptr->require_version->version;
}

}  // namespace internal
}  // namespace mojo
