// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>

#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/nacl/nonsfi/nexe_launcher_nonsfi.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"

namespace nacl {
namespace content_handler {

class NaClContentHandler : public mojo::ApplicationImplBase,
                           public mojo::ContentHandlerFactory::Delegate {
 public:
  NaClContentHandler() {}

 private:
  // Overridden from ApplicationImplBase:
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<mojo::ContentHandler>(
        mojo::ContentHandlerFactory::GetInterfaceRequestHandler(this));
    return true;
  }

  // Overridden from ContentHandlerFactory::Delegate:
  void RunApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override {
    // Needed to use Mojo interfaces on this thread.
    base::MessageLoop loop(mojo::common::MessagePumpMojo::Create());
    // Acquire the nexe.
    base::ScopedFILE nexe_fp =
        mojo::common::BlockingCopyToTempFile(response->body.Pass());
    CHECK(nexe_fp)  << "Could not redirect nexe to temp file";
    FILE* nexe_file_stream = nexe_fp.release();
    int fd = fileno(nexe_file_stream);
    CHECK_NE(fd, -1) << "Could not open the stream pointer's file descriptor";
    fd = dup(fd);
    CHECK_NE(fd, -1) << "Could not dup the file descriptor";
    CHECK_EQ(fclose(nexe_file_stream), 0) << "Failed to close temp file";

    MojoHandle handle =
        application_request.PassMessagePipe().release().value();
    // MojoLaunchNexeNonsfi takes ownership of the fd.
    MojoLaunchNexeNonsfi(fd, handle, false /* enable_translation_irt */);
  }

  DISALLOW_COPY_AND_ASSIGN(NaClContentHandler);
};

}  // namespace content_handler
}  // namespace nacl

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  nacl::content_handler::NaClContentHandler nacl_content_handler;
  return mojo::RunApplication(application_request, &nacl_content_handler);
}
