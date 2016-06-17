// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/threading/sequenced_worker_pool.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "services/asset_bundle/asset_unpacker_impl.h"

namespace mojo {
namespace asset_bundle {

class AssetBundleApp : public ApplicationImplBase {
 public:
  AssetBundleApp() {}
  ~AssetBundleApp() override {}

 private:
  // |ApplicationImplBase| override:
  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<AssetUnpacker>(
        [this](const ConnectionContext& connection_context,
               InterfaceRequest<AssetUnpacker> asset_unpacker_request) {
          // Lazily initialize |sequenced_worker_pool_|. (We can't create it in
          // the constructor, since the message loop (which implies that there's
          // a current SingleThreadTaskRunner) is only created in
          // mojo::RunApplication().)
          if (!sequenced_worker_pool_) {
            // TODO(vtl): What's the "right" way to choose the maximum number of
            // threads?
            sequenced_worker_pool_ =
                new base::SequencedWorkerPool(4, "AssetBundleWorker");
          }

          new AssetUnpackerImpl(
              asset_unpacker_request.Pass(),
              sequenced_worker_pool_->GetTaskRunnerWithShutdownBehavior(
                  base::SequencedWorkerPool::SKIP_ON_SHUTDOWN));
        });
    return true;
  }

  // We don't really need the "sequenced" part, but we need to be able to shut
  // down our worker pool.
  scoped_refptr<base::SequencedWorkerPool> sequenced_worker_pool_;

  DISALLOW_COPY_AND_ASSIGN(AssetBundleApp);
};

}  // namespace asset_bundle
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  mojo::asset_bundle::AssetBundleApp asset_bundle_app;
  return mojo::RunApplication(application_request, &asset_bundle_app);
}
