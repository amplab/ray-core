// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_support/process_io_redirection.h"

#include "base/logging.h"

namespace native_support {

ProcessIORedirectionForStdIO::ProcessIORedirectionForStdIO(
    mojo::files::FilePtr stdin_file,
    mojo::files::FilePtr stdout_file,
    mojo::files::FilePtr stderr_file,
    base::ScopedFD stdin_fd,
    base::ScopedFD stdout_fd,
    base::ScopedFD stderr_fd)
    : stdin_file_(stdin_file.Pass()),
      stdout_file_(stdout_file.Pass()),
      stderr_file_(stderr_file.Pass()),
      stdin_fd_(stdin_fd.Pass()),
      stdout_fd_(stdout_fd.Pass()),
      stderr_fd_(stderr_fd.Pass()) {
  if (stdin_file_) {
    stdin_redirector_.reset(
        new MojoFileToFDRedirector(stdin_file_.get(), stdin_fd_.get(), 4096));
    stdin_redirector_->Start();
  }
  if (stdout_file_) {
    stdout_redirector_.reset(
        new FDToMojoFileRedirector(stdout_fd_.get(), stdout_file_.get(), 4096));
    stdout_redirector_->Start();
  }
  if (stderr_file_) {
    stderr_redirector_.reset(
        new FDToMojoFileRedirector(stderr_fd_.get(), stderr_file_.get(), 4096));
    stderr_redirector_->Start();
  }
}

ProcessIORedirectionForStdIO::~ProcessIORedirectionForStdIO() {}

ProcessIORedirectionForTerminal::ProcessIORedirectionForTerminal(
    mojo::files::FilePtr terminal_file,
    base::ScopedFD terminal_fd)
    : terminal_file_(terminal_file.Pass()),
      terminal_fd_(terminal_fd.Pass()),
      input_redirector_(terminal_file_.get(), terminal_fd_.get(), 4096),
      output_redirector_(terminal_fd_.get(), terminal_file_.get(), 4096) {
  input_redirector_.Start();
  output_redirector_.Start();
}

ProcessIORedirectionForTerminal::~ProcessIORedirectionForTerminal() {}

}  // namespace native_support
