// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "mojo/nacl/nonsfi/file_util.h"
#include "mojo/nacl/nonsfi/nexe_launcher_nonsfi.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/nacl/nonsfi/kLdNexe.h"
#include "services/nacl/nonsfi/pnacl_link.mojom.h"

namespace mojo {
namespace nacl {

class PexeLinkerImpl : public PexeLinkerInit {
 public:
  void PexeLinkerStart(InterfaceRequest<PexeLinker> linker_request) override {
    int nexe_fd = ::nacl::DataToTempFileDescriptor(::nacl::kLdNexe);
    CHECK(nexe_fd >= 0) << "Could not open linker nexe";
    ::nacl::MojoLaunchNexeNonsfi(
        nexe_fd, linker_request.PassMessagePipe().release().value(),
        true /* enable_translate_irt */);
  }
};

class StrongBindingPexeLinkerImpl : public PexeLinkerImpl {
 public:
  explicit StrongBindingPexeLinkerImpl(InterfaceRequest<PexeLinkerInit> request)
      : strong_binding_(this, request.Pass()) {}

 private:
  StrongBinding<PexeLinkerInit> strong_binding_;
};

class MultiPexeLinker : public ApplicationImplBase {
 public:
  MultiPexeLinker() {}

  // From ApplicationImplBase
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<PexeLinkerInit>(
        [](const ConnectionContext& connection_context,
           InterfaceRequest<PexeLinkerInit> request) {
          new StrongBindingPexeLinkerImpl(request.Pass());
        });
    return true;
  }
};

}  // namespace nacl
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::nacl::MultiPexeLinker multi_pexe_linker;
  return mojo::RunApplication(application_request, &multi_pexe_linker);
}
