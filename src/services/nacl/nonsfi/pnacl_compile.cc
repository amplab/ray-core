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
#include "services/nacl/nonsfi/kPnaclTranslatorCompile.h"
#include "services/nacl/nonsfi/pnacl_compile.mojom.h"

namespace mojo {
namespace nacl {

class PexeCompilerImpl : public PexeCompilerInit {
 public:
  void PexeCompilerStart(
      InterfaceRequest<PexeCompiler> compiler_request) override {
    int nexe_fd =
        ::nacl::DataToTempFileDescriptor(::nacl::kPnaclTranslatorCompile);
    CHECK(nexe_fd >= 0) << "Could not open compiler nexe";
    ::nacl::MojoLaunchNexeNonsfi(
        nexe_fd, compiler_request.PassMessagePipe().release().value(),
        true /* enable_translate_irt */);
  }
};

class StrongBindingPexeCompilerImpl : public PexeCompilerImpl {
 public:
  explicit StrongBindingPexeCompilerImpl(InterfaceRequest<PexeCompilerInit>
                                         request)
      : strong_binding_(this, request.Pass()) {}

 private:
  StrongBinding<PexeCompilerInit> strong_binding_;
};

class MultiPexeCompiler : public ApplicationImplBase {
 public:
  MultiPexeCompiler() {}

  // From ApplicationImplBase
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<PexeCompilerInit>(
        [](const ConnectionContext& connection_context,
           InterfaceRequest<PexeCompilerInit> request) {
          new StrongBindingPexeCompilerImpl(request.Pass());
        });
    return true;
  }
};

}  // namespace nacl
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::nacl::MultiPexeCompiler multi_pexe_compiler;
  return mojo::RunApplication(application_request, &multi_pexe_compiler);
}
