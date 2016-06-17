// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a terminal client (i.e., a "raw" |mojo.terminal.Terminal| -- e.g.,
// moterm -- can be asked to talk to this) that prompts the user for a native
// (Linux) binary to run and then does so (via mojo:native_support).
//
// E.g., first run mojo:moterm_example_app (embedded by a window manager). Then,
// at the prompt, enter "mojo:native_run_app". At the next prompt, enter "bash"
// (or "echo hello mojo").
//
// TODO(vtl): Maybe it should optionally be able to extract the binary path (and
// args) from the connection URL?

#include <string.h>

#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/files/interfaces/files.mojom.h"
#include "mojo/services/files/interfaces/ioctl.mojom.h"
#include "mojo/services/files/interfaces/ioctl_terminal.mojom.h"
#include "mojo/services/files/interfaces/types.mojom.h"
#include "mojo/services/native_support/interfaces/process.mojom.h"
#include "mojo/services/terminal/interfaces/terminal_client.mojom.h"

using mojo::terminal::TerminalClient;

mojo::Array<uint8_t> ToByteArray(const std::string& s) {
  auto rv = mojo::Array<uint8_t>::New(s.size());
  memcpy(rv.data(), s.data(), s.size());
  return rv;
}

class TerminalConnection {
 public:
  explicit TerminalConnection(mojo::InterfaceHandle<mojo::files::File> terminal,
                              native_support::Process* native_support_process)
      : terminal_(mojo::files::FilePtr::Create(std::move(terminal))),
        native_support_process_(native_support_process) {
    terminal_.set_connection_error_handler([this]() { delete this; });
    Start();
  }
  ~TerminalConnection() {}

 private:
  void Write(const char* s, mojo::files::File::WriteCallback callback) {
    size_t length = strlen(s);
    auto a = mojo::Array<uint8_t>::New(length);
    memcpy(&a[0], s, length);
    terminal_->Write(a.Pass(), 0, mojo::files::Whence::FROM_CURRENT, callback);
  }

  void Start() {
    // TODO(vtl): Check canonical mode (via ioctl) first (or before |Read()|).

    const char kPrompt[] = "\x1b[0mNative program to run?\n>>> ";
    Write(kPrompt, [this](mojo::files::Error error, uint32_t) {
      this->DidWritePrompt(error);
    });
  }
  void DidWritePrompt(mojo::files::Error error) {
    if (error != mojo::files::Error::OK) {
      LOG(ERROR) << "Write() error: " << error;
      delete this;
      return;
    }

    terminal_->Read(
        1000, 0, mojo::files::Whence::FROM_CURRENT,
        [this](mojo::files::Error error, mojo::Array<uint8_t> bytes_read) {
          this->DidReadFromPrompt(error, bytes_read.Pass());
        });
  }
  void DidReadFromPrompt(mojo::files::Error error,
                         mojo::Array<uint8_t> bytes_read) {
    if (error != mojo::files::Error::OK || !bytes_read.size()) {
      LOG(ERROR) << "Read() error: " << error;
      delete this;
      return;
    }

    std::string input(reinterpret_cast<const char*>(&bytes_read[0]),
                      bytes_read.size());
    // TODO(vtl): Are these arguments really what we want? Probably not.
    command_line_ =
        base::SplitString(input, base::kWhitespaceASCII, base::KEEP_WHITESPACE,
                          base::SPLIT_WANT_NONEMPTY);

    if (command_line_.empty()) {
      Start();
      return;
    }

    // Set the terminal to noncanonical mode. Do so by getting the settings,
    // flipping the flag, and setting them.
    // TODO(vtl): Should it do other things?
    // TODO(vtl): I wonder if these ioctls shouldn't be done by the
    // |SpawnWithTerminal()| implementation instead. Hmmm.
    mojo::Array<uint32_t> in_values = mojo::Array<uint32_t>::New(1);
    in_values[0] = mojo::files::kIoctlTerminalGetSettings;
    terminal_->Ioctl(
        mojo::files::kIoctlTerminal, in_values.Pass(),
        [this](mojo::files::Error error, mojo::Array<uint32_t> out_values) {
          this->DidGetTerminalSettings(error, out_values.Pass());
        });
  }
  void DidGetTerminalSettings(mojo::files::Error error,
                              mojo::Array<uint32_t> out_values) {
    if (error != mojo::files::Error::OK || out_values.size() < 6) {
      LOG(ERROR) << "Ioctl() (terminal get settings) error: " << error;
      delete this;
      return;
    }

    const size_t kBaseFieldCount =
        mojo::files::kIoctlTerminalTermiosBaseFieldCount;
    const uint32_t kLFlagIdx = mojo::files::kIoctlTerminalTermiosLFlagIndex;
    const uint32_t kLFlagICANON = mojo::files::kIoctlTerminalTermiosLFlagICANON;

    auto in_values = mojo::Array<uint32_t>::New(1 + kBaseFieldCount);
    in_values[0] = mojo::files::kIoctlTerminalSetSettings;
    for (size_t i = 0; i < kBaseFieldCount; i++)
      in_values[1 + i] = out_values[i];
    // Just turn off ICANON, which is in "lflag".
    in_values[1 + kLFlagIdx] &= ~kLFlagICANON;
    terminal_->Ioctl(
        mojo::files::kIoctlTerminal, in_values.Pass(),
        [this](mojo::files::Error error, mojo::Array<uint32_t> out_values) {
          this->DidSetTerminalSettings(error, out_values.Pass());
        });
  }
  void DidSetTerminalSettings(mojo::files::Error error,
                              mojo::Array<uint32_t> out_values) {
    if (error != mojo::files::Error::OK) {
      LOG(ERROR) << "Ioctl() (terminal set settings) error: " << error;
      delete this;
      return;
    }

    // Now, we can spawn.
    mojo::Array<mojo::Array<uint8_t>> argv;
    for (const auto& arg : command_line_)
      argv.push_back(ToByteArray(arg));

    // TODO(vtl): If the |InterfacePtr| underlying |native_support_process_|
    // encounters an error, then we're sort of dead in the water.
    native_support_process_->SpawnWithTerminal(
        ToByteArray(command_line_[0]), argv.Pass(), nullptr,
        terminal_.PassInterfaceHandle(), GetProxy(&process_controller_),
        [this](mojo::files::Error error) {
          this->DidSpawnWithTerminal(error);
        });
    process_controller_.set_connection_error_handler([this]() { delete this; });
  }
  void DidSpawnWithTerminal(mojo::files::Error error) {
    if (error != mojo::files::Error::OK) {
      LOG(ERROR) << "SpawnWithTerminal() error: " << error;
      delete this;
      return;
    }
    process_controller_->Wait(
        [this](mojo::files::Error error, int32_t exit_status) {
          this->DidWait(error, exit_status);
        });
  }
  void DidWait(mojo::files::Error error, int32_t exit_status) {
    if (error != mojo::files::Error::OK)
      LOG(ERROR) << "Wait() error: " << error;
    else if (exit_status != 0)  // |exit_status| only valid if OK.
      LOG(ERROR) << "Process exit status: " << exit_status;

    // We're done, regardless.
    delete this;
  }

  mojo::files::FilePtr terminal_;
  native_support::Process* native_support_process_;
  native_support::ProcessControllerPtr process_controller_;

  std::vector<std::string> command_line_;

  DISALLOW_COPY_AND_ASSIGN(TerminalConnection);
};

class TerminalClientImpl : public TerminalClient {
 public:
  TerminalClientImpl(mojo::InterfaceRequest<TerminalClient> request,
                     native_support::Process* native_support_process)
      : binding_(this, request.Pass()),
        native_support_process_(native_support_process) {}
  ~TerminalClientImpl() override {}

  // |TerminalClient| implementation:
  void ConnectToTerminal(
      mojo::InterfaceHandle<mojo::files::File> terminal) override {
    if (terminal) {
      // Owns itself.
      new TerminalConnection(std::move(terminal), native_support_process_);
    } else {
      LOG(ERROR) << "No terminal";
    }
  }

 private:
  mojo::StrongBinding<TerminalClient> binding_;
  native_support::Process* native_support_process_;

  DISALLOW_COPY_AND_ASSIGN(TerminalClientImpl);
};

class NativeRunApp : public mojo::ApplicationImplBase {
 public:
  NativeRunApp() {}
  ~NativeRunApp() override {}

 private:
  // |mojo::ApplicationImplBase| overrides:
  void OnInitialize() override {
    mojo::ConnectToService(shell(), "mojo:native_support",
                           GetProxy(&native_support_process_));
  }

  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<TerminalClient>(
        [this](const mojo::ConnectionContext& connection_context,
               mojo::InterfaceRequest<TerminalClient> terminal_client_request) {
          new TerminalClientImpl(terminal_client_request.Pass(),
                                 native_support_process_.get());
        });
    return true;
  }

  native_support::ProcessPtr native_support_process_;

  DISALLOW_COPY_AND_ASSIGN(NativeRunApp);
};

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  NativeRunApp native_run_app;
  return mojo::RunApplication(application_request, &native_run_app);
}
