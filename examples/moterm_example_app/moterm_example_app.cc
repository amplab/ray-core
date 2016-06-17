// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a simple example application (with an embeddable view), which embeds
// the Moterm view, uses it to prompt the user, etc.

#include <string.h>

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "mojo/common/binding_set.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/bindings/array.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "mojo/services/files/interfaces/file.mojom.h"
#include "mojo/services/files/interfaces/types.mojom.h"
#include "mojo/services/terminal/interfaces/terminal.mojom.h"
#include "mojo/services/terminal/interfaces/terminal_client.mojom.h"
#include "mojo/services/ui/views/interfaces/view_manager.mojom.h"
#include "mojo/services/ui/views/interfaces/view_provider.mojom.h"
#include "mojo/services/ui/views/interfaces/views.mojom.h"
#include "mojo/ui/view_provider_app.h"

// Kind of like |fputs()| (doesn't wait for result).
void Fputs(mojo::files::File* file, const char* s) {
  size_t length = strlen(s);
  auto a = mojo::Array<uint8_t>::New(length);
  memcpy(&a[0], s, length);

  file->Write(a.Pass(), 0, mojo::files::Whence::FROM_CURRENT,
              mojo::files::File::WriteCallback());
}

class MotermExampleAppView {
 public:
  MotermExampleAppView(
      mojo::Shell* shell,
      mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request)
      : shell_(shell), weak_factory_(this) {
    // Connect to the moterm app.
    LOG(INFO) << "Connecting to moterm";
    mojo::ServiceProviderPtr moterm_app;
    shell->ConnectToApplication("mojo:moterm", GetProxy(&moterm_app));

    // Create the moterm view and pass it back to the client directly.
    mojo::ConnectToService(moterm_app.get(), GetProxy(&moterm_view_provider_));
    mojo::ServiceProviderPtr moterm_service_provider;
    moterm_view_provider_->CreateView(view_owner_request.Pass(),
                                      GetProxy(&moterm_service_provider));

    // Connect to the moterm terminal service associated with the view
    // we just created.
    mojo::ConnectToService(moterm_service_provider.get(),
                           GetProxy(&moterm_terminal_));

    // Start running.
    StartPrompt(true);
  }

  ~MotermExampleAppView() {}

 private:
  void Resize() {
    moterm_terminal_->SetSize(0, 0, false, [](mojo::files::Error error,
                                              uint32_t rows, uint32_t columns) {
      DCHECK_EQ(error, mojo::files::Error::OK);
      DVLOG(1) << "New size: " << rows << "x" << columns;
    });
  }

  void StartPrompt(bool first_time) {
    if (!moterm_file_) {
      moterm_terminal_->Connect(GetProxy(&moterm_file_), false,
                                [](mojo::files::Error error) {
                                  DCHECK_EQ(error, mojo::files::Error::OK);
                                });
    }

    if (first_time) {
      // Display some welcome text.
      Fputs(moterm_file_.get(),
            "Welcome to "
            "\x1b[1m\x1b[34mM\x1b[31mo\x1b[33mt\x1b[34me\x1b[32mr\x1b[31mm\n"
            "\n");
    }

    Fputs(moterm_file_.get(), "\x1b[0m\nWhere do you want to go today?\n:) ");
    moterm_file_->Read(1000, 0, mojo::files::Whence::FROM_CURRENT,
                       base::Bind(&MotermExampleAppView::OnInputFromPrompt,
                                  weak_factory_.GetWeakPtr()));
  }
  void OnInputFromPrompt(mojo::files::Error error,
                         mojo::Array<uint8_t> bytes_read) {
    if (error != mojo::files::Error::OK || !bytes_read) {
      // TODO(vtl): Handle errors?
      NOTIMPLEMENTED();
      return;
    }

    std::string dest_url;
    if (bytes_read.size() >= 1) {
      base::TrimWhitespaceASCII(
          std::string(reinterpret_cast<const char*>(&bytes_read[0]),
                      bytes_read.size()),
          base::TRIM_ALL, &dest_url);
    }

    if (dest_url.empty()) {
      Fputs(moterm_file_.get(), "\nError: no destination URL given\n");
      StartPrompt(false);
      return;
    }

    Fputs(moterm_file_.get(),
          base::StringPrintf("\nGoing to %s ...\n", dest_url.c_str()).c_str());
    moterm_file_.reset();

    mojo::ServiceProviderPtr dest_sp;
    shell_->ConnectToApplication(dest_url, GetProxy(&dest_sp));
    mojo::terminal::TerminalClientPtr dest_terminal_client;
    mojo::ConnectToService(dest_sp.get(), GetProxy(&dest_terminal_client));
    moterm_terminal_->ConnectToClient(
        std::move(dest_terminal_client), true,
        base::Bind(&MotermExampleAppView::OnDestinationDone,
                   weak_factory_.GetWeakPtr()));
  }
  void OnDestinationDone(mojo::files::Error error) {
    // We should always succeed. (It'll only fail on synchronous failures, which
    // only occur when it's busy.)
    DCHECK_EQ(error, mojo::files::Error::OK);
    StartPrompt(false);
  }

  mojo::Shell* const shell_;
  mojo::ui::ViewProviderPtr moterm_view_provider_;

  mojo::terminal::TerminalPtr moterm_terminal_;
  // Valid while prompting.
  mojo::files::FilePtr moterm_file_;

  base::WeakPtrFactory<MotermExampleAppView> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MotermExampleAppView);
};

class MotermExampleApp : public mojo::ui::ViewProviderApp {
 public:
  MotermExampleApp() {}
  ~MotermExampleApp() override {}

  // |ViewProviderApp|:
  void CreateView(
      const std::string& connection_url,
      mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
      mojo::InterfaceRequest<mojo::ServiceProvider> services) override {
    new MotermExampleAppView(shell(), view_owner_request.Pass());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MotermExampleApp);
};

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  MotermExampleApp moterm_example_app;
  return mojo::RunApplication(application_request, &moterm_example_app);
}
