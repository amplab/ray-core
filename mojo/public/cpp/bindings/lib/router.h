// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_ROUTER_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_ROUTER_H_

#include <map>

#include "mojo/public/cpp/bindings/callback.h"
#include "mojo/public/cpp/bindings/lib/connector.h"
#include "mojo/public/cpp/bindings/lib/shared_data.h"
#include "mojo/public/cpp/bindings/lib/validation_errors.h"
#include "mojo/public/cpp/bindings/message_validator.h"
#include "mojo/public/cpp/environment/environment.h"

namespace mojo {
namespace internal {

// Router provides a way for sending messages over a MessagePipe, and re-routing
// response messages back to the sender.
class Router : public MessageReceiverWithResponder {
 public:
  Router(ScopedMessagePipeHandle message_pipe,
         MessageValidatorList validators,
         const MojoAsyncWaiter* waiter = Environment::GetDefaultAsyncWaiter());
  ~Router() override;

  // Sets the receiver to handle messages read from the message pipe that do
  // not have the kMessageIsResponse flag set.
  void set_incoming_receiver(MessageReceiverWithResponderStatus* receiver) {
    incoming_receiver_ = receiver;
  }

  // Sets the error handler to receive notifications when an error is
  // encountered while reading from the pipe or waiting to read from the pipe.
  void set_connection_error_handler(const Closure& error_handler) {
    connector_.set_connection_error_handler(error_handler);
  }

  // Returns true if an error was encountered while reading from the pipe or
  // waiting to read from the pipe.
  bool encountered_error() const { return connector_.encountered_error(); }

  // Is the router bound to a MessagePipe handle?
  bool is_valid() const { return connector_.is_valid(); }

  void CloseMessagePipe() { connector_.CloseMessagePipe(); }

  ScopedMessagePipeHandle PassMessagePipe() {
    return connector_.PassMessagePipe();
  }

  // MessageReceiver implementation:
  bool Accept(Message* message) override;
  bool AcceptWithResponder(Message* message,
                           MessageReceiver* responder) override;

  // Blocks the current thread until the first incoming method call, i.e.,
  // either a call to a client method or a callback method, or |deadline|.
  // When returning |false| closes the message pipe, unless the reason for
  // for returning |false| was |MOJO_RESULT_SHOULD_WAIT| or
  // |MOJO_RESULT_DEADLINE_EXCEEDED|.
  // Use |encountered_error| to see if an error occurred.
  bool WaitForIncomingMessage(MojoDeadline deadline) {
    return connector_.WaitForIncomingMessage(deadline);
  }

  // Sets this object to testing mode.
  // In testing mode:
  // - the object is more tolerant of unrecognized response messages;
  // - the connector continues working after seeing errors from its incoming
  //   receiver.
  void EnableTestingMode();

  MessagePipeHandle handle() const { return connector_.handle(); }

 private:
  typedef std::map<uint64_t, MessageReceiver*> ResponderMap;

  // This class is registered for incoming messages from the |Connector|.  It
  // simply forwards them to |Router::HandleIncomingMessages|.
  class HandleIncomingMessageThunk : public MessageReceiver {
   public:
    explicit HandleIncomingMessageThunk(Router* router);
    ~HandleIncomingMessageThunk() override;

    // MessageReceiver implementation:
    bool Accept(Message* message) override;

   private:
    Router* router_;
  };

  bool HandleIncomingMessage(Message* message);

  HandleIncomingMessageThunk thunk_;
  MessageValidatorList validators_;
  Connector connector_;
  SharedData<Router*> weak_self_;
  MessageReceiverWithResponderStatus* incoming_receiver_;
  ResponderMap responders_;
  uint64_t next_request_id_;
  bool testing_mode_;
};

}  // namespace internal
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_ROUTER_H_
