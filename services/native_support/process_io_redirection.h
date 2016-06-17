// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_SUPPORT_PROCESS_IO_REDIRECTION_H_
#define SERVICES_NATIVE_SUPPORT_PROCESS_IO_REDIRECTION_H_

#include <memory>

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "mojo/services/files/interfaces/file.mojom.h"
#include "services/native_support/redirectors.h"

namespace native_support {

class ProcessIORedirection {
 public:
  virtual ~ProcessIORedirection() {}

 protected:
  ProcessIORedirection() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ProcessIORedirection);
};

class ProcessIORedirectionForStdIO : public ProcessIORedirection {
 public:
  ProcessIORedirectionForStdIO(mojo::files::FilePtr stdin_file,
                               mojo::files::FilePtr stdout_file,
                               mojo::files::FilePtr stderr_file,
                               base::ScopedFD stdin_fd,
                               base::ScopedFD stdout_fd,
                               base::ScopedFD stderr_fd);
  ~ProcessIORedirectionForStdIO() override;

 private:
  mojo::files::FilePtr stdin_file_;
  mojo::files::FilePtr stdout_file_;
  mojo::files::FilePtr stderr_file_;
  base::ScopedFD stdin_fd_;
  base::ScopedFD stdout_fd_;
  base::ScopedFD stderr_fd_;

  // The above things must outlive these.
  std::unique_ptr<MojoFileToFDRedirector> stdin_redirector_;
  std::unique_ptr<FDToMojoFileRedirector> stdout_redirector_;
  std::unique_ptr<FDToMojoFileRedirector> stderr_redirector_;

  DISALLOW_COPY_AND_ASSIGN(ProcessIORedirectionForStdIO);
};

class ProcessIORedirectionForTerminal : public ProcessIORedirection {
 public:
  ProcessIORedirectionForTerminal(mojo::files::FilePtr terminal_file,
                                  base::ScopedFD terminal_fd);
  ~ProcessIORedirectionForTerminal() override;

 private:
  mojo::files::FilePtr terminal_file_;
  base::ScopedFD terminal_fd_;

  // The above things must outlive these.
  // TODO(vtl): We should have a bidirectional redirector instead.
  MojoFileToFDRedirector input_redirector_;
  FDToMojoFileRedirector output_redirector_;

  DISALLOW_COPY_AND_ASSIGN(ProcessIORedirectionForTerminal);
};

}  // namespace native_support

#endif  // SERVICES_NATIVE_SUPPORT_PROCESS_IO_REDIRECTION_H_
