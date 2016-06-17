// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "mojo/nacl/nonsfi/irt_mojo_nonsfi.h"
#include "mojo/public/cpp/bindings/array.h"
#include "mojo/public/cpp/bindings/string.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "services/nacl/nonsfi/pnacl_link.mojom.h"

namespace {

typedef int (*LinkerCallback)(int, const int*, int);

class PexeLinkerImpl : public mojo::nacl::PexeLinker {
 public:
  PexeLinkerImpl(mojo::ScopedMessagePipeHandle handle, LinkerCallback func)
      : func_(func), strong_binding_(this, handle.Pass()) {}
  void PexeLink(mojo::Array<mojo::String> obj_file_names,
                const mojo::Callback<void(mojo::String)>& callback)
      override {
    // Create a temporary .nexe file which will be the result of calling our
    // linker.
    base::FilePath nexe_file_name;
    CHECK(CreateTemporaryFile(&nexe_file_name))
        << "Could not create temporary nexe file";
    int nexe_file_fd = open(nexe_file_name.value().c_str(), O_RDWR);
    CHECK_GE(nexe_file_fd, 0) << "Could not create temp file for linked nexe";

    // Open our temporary object file. Additionally, unlink it, since it is a
    // temporary file that is no longer needed after it is opened.
    size_t obj_file_fd_count = obj_file_names.size();
    int obj_file_fds[obj_file_fd_count];
    for (size_t i = 0; i < obj_file_fd_count; i++) {
      obj_file_fds[i] = open(obj_file_names[i].get().c_str(), O_RDONLY);
      CHECK(!unlink(obj_file_names[i].get().c_str()))
          << "Could not unlink temporary object file";
      CHECK_GE(obj_file_fds[i], 0) << "Could not open object file";
    }

    CHECK(!func_(nexe_file_fd, obj_file_fds, obj_file_fd_count))
        << "Error calling func on object file";

    // Return the name of the nexe file.
    callback.Run(mojo::String(nexe_file_name.value()));
    mojo::RunLoop::current()->Quit();
  }
 private:
  LinkerCallback func_;
  mojo::StrongBinding<mojo::nacl::PexeLinker> strong_binding_;
};

void ServeLinkRequest(LinkerCallback func) {
  // Acquire the handle -- this is our mechanism to contact the
  // content handler which called us.
  MojoHandle handle;
  nacl::MojoGetInitialHandle(&handle);

  // Convert the MojoHandle into a ScopedMessagePipeHandle, and use that to
  // implement the PexeLinker interface.
  PexeLinkerImpl impl(
      mojo::ScopedMessagePipeHandle(mojo::MessagePipeHandle(handle)).Pass(),
      func);
  mojo::RunLoop::current()->Run();
}

}  // namespace anonymous

namespace nacl {

const struct nacl_irt_private_pnacl_translator_link
    nacl_irt_private_pnacl_translator_link = {
  ServeLinkRequest
};

}  // namespace nacl
