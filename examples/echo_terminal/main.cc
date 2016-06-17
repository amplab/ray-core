// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/macros.h"
#include "mojo/common/binding_set.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/files/interfaces/file.mojom.h"
#include "mojo/services/files/interfaces/types.mojom.h"
#include "mojo/services/terminal/interfaces/terminal_client.mojom.h"

const uint32_t kMaxBytesToRead = 1000;

// Owns itself.
class TerminalEchoer {
 public:
  explicit TerminalEchoer(mojo::files::FilePtr terminal)
      : terminal_(terminal.Pass()), last_bytes_read_size_(0) {
    terminal_.set_connection_error_handler([this]() { OnConnectionError(); });
  }

  void StartReading() {
    // TODO(vtl): Are |offset| and |whence| correct?
    terminal_->Read(
        kMaxBytesToRead, 0, mojo::files::Whence::FROM_CURRENT,
        base::Bind(&TerminalEchoer::OnRead, base::Unretained(this)));
  }

 private:
  ~TerminalEchoer() {}

  // Error callback:
  void OnConnectionError() { delete this; }

  // |Read()| callback:
  void OnRead(mojo::files::Error error, mojo::Array<uint8_t> bytes_read) {
    if (error != mojo::files::Error::OK) {
      LOG(ERROR) << "Error: Read(): " << error;
      delete this;
      return;
    }

    if (!bytes_read) {
      LOG(ERROR) << "Error: no bytes read (null)";
      delete this;
      return;
    }

    if (bytes_read.size() == 0 || bytes_read.size() > kMaxBytesToRead) {
      LOG(ERROR) << "Error: invalid amount of bytes read: " << bytes_read.size()
                 << " bytes";
      delete this;
      return;
    }

    // Save this, so that we can check in |OnWrite()|.
    last_bytes_read_size_ = bytes_read.size();

    // TODO(vtl): Are |offset| and |whence| correct?
    terminal_->Write(
        bytes_read.Pass(), 0, mojo::files::Whence::FROM_CURRENT,
        base::Bind(&TerminalEchoer::OnWrite, base::Unretained(this)));
  }

  // |Write()| callback:
  void OnWrite(mojo::files::Error error, uint32_t num_bytes_written) {
    if (error != mojo::files::Error::OK) {
      LOG(ERROR) << "Error: Write(): " << error;
      delete this;
      return;
    }

    if (num_bytes_written != last_bytes_read_size_) {
      LOG(ERROR) << "Error: failed to write all bytes (last read: "
                 << last_bytes_read_size_
                 << " bytes; wrote: " << num_bytes_written << " bytes)";
      delete this;
      return;
    }

    StartReading();
  }

  mojo::files::FilePtr terminal_;
  size_t last_bytes_read_size_;

  DISALLOW_COPY_AND_ASSIGN(TerminalEchoer);
};

class EchoTerminalApp : public mojo::ApplicationImplBase,
                        public mojo::terminal::TerminalClient {
 public:
  EchoTerminalApp() {}
  ~EchoTerminalApp() override {}

 private:
  // |ApplicationImplBase| override:
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<mojo::terminal::TerminalClient>(
        [this](const mojo::ConnectionContext& connection_context,
               mojo::InterfaceRequest<mojo::terminal::TerminalClient>
                   terminal_client_request) {
          terminal_clients_.AddBinding(this, terminal_client_request.Pass());
        });
    return true;
  }

  // |mojo::terminal::TerminalClient| implementation:
  void ConnectToTerminal(
      mojo::InterfaceHandle<mojo::files::File> terminal) override {
    DCHECK(terminal);
    // The |TerminalEchoer| will own itself.
    (new TerminalEchoer(mojo::files::FilePtr::Create(std::move(terminal))))
        ->StartReading();
  }

  mojo::BindingSet<mojo::terminal::TerminalClient> terminal_clients_;

  DISALLOW_COPY_AND_ASSIGN(EchoTerminalApp);
};

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  EchoTerminalApp echo_terminal_app;
  return mojo::RunApplication(application_request, &echo_terminal_app);
}
