// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/sequenced_worker_pool.h"
#include "mojo/application/run_application_options_chromium.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/native_support/interfaces/process.mojom.h"
#include "services/native_support/process_impl.h"

namespace native_support {

// TODO(vtl): Having to manually choose an arbitrary number is dumb.
const size_t kMaxWorkerThreads = 16;

class NativeSupportApp : public mojo::ApplicationImplBase {
 public:
  NativeSupportApp() {}
  ~NativeSupportApp() override {
    if (worker_pool_)
      worker_pool_->Shutdown();
  }

 private:
  // |mojo::ApplicationImplBase| overrides:
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<Process>([this](
        const mojo::ConnectionContext& connection_context,
        mojo::InterfaceRequest<Process> process_request) {
      if (!worker_pool_) {
        worker_pool_ = new base::SequencedWorkerPool(kMaxWorkerThreads,
                                                     "NativeSupportWorker");
      }
      new ProcessImpl(worker_pool_, connection_context, process_request.Pass());
    });
    return true;
  }

  scoped_refptr<base::SequencedWorkerPool> worker_pool_;

  DISALLOW_COPY_AND_ASSIGN(NativeSupportApp);
};

}  // namespace native_support

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  native_support::NativeSupportApp native_support_app;
  // We need an I/O message loop, since we'll want to watch FDs.
  mojo::RunApplicationOptionsChromium options(base::MessageLoop::TYPE_IO);
  return mojo::RunApplication(application_request, &native_support_app,
                              &options);
}
