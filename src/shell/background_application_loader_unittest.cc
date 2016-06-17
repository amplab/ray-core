// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/background_application_loader.h"

#include "mojo/public/interfaces/application/application.mojom.h"
#include "shell/context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace shell {
namespace {

class DummyLoader : public ApplicationLoader {
 public:
  DummyLoader() : simulate_app_quit_(true) {}
  ~DummyLoader() override {}

  // ApplicationLoader overrides:
  void Load(
      const GURL& url,
      mojo::InterfaceRequest<mojo::Application> application_request) override {
    if (simulate_app_quit_)
      base::MessageLoop::current()->Quit();
  }

  void DontSimulateAppQuit() { simulate_app_quit_ = false; }

 private:
  bool simulate_app_quit_;
};

// Tests that the loader can start and stop gracefully.
TEST(BackgroundApplicationLoaderTest, StartStop) {
  Context::EnsureEmbedderIsInitialized();
  scoped_ptr<ApplicationLoader> real_loader(new DummyLoader());
  BackgroundApplicationLoader loader(real_loader.Pass(), "test",
                                     base::MessageLoop::TYPE_DEFAULT);
}

// Tests that the loader can load a service that is well behaved (quits
// itself).
TEST(BackgroundApplicationLoaderTest, Load) {
  Context::EnsureEmbedderIsInitialized();
  scoped_ptr<ApplicationLoader> real_loader(new DummyLoader());
  BackgroundApplicationLoader loader(real_loader.Pass(), "test",
                                     base::MessageLoop::TYPE_DEFAULT);
  mojo::ApplicationPtr application;
  loader.Load(GURL(), mojo::GetProxy(&application));
}

}  // namespace
}  // namespace shell
