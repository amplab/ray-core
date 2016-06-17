// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/scoped_temp_dir.h"
#include "shell/application_manager/application_manager.h"
#include "shell/application_manager/native_application_options.h"
#include "shell/context.h"
#include "shell/filename_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace shell {
namespace {

struct TestState {
  TestState()
      : runner_was_created(false),
        runner_was_started(false),
        runner_was_destroyed(false) {}

  bool runner_was_created;
  bool runner_was_started;
  bool runner_was_destroyed;
};

class TestNativeRunner : public NativeRunner {
 public:
  explicit TestNativeRunner(TestState* state) : state_(state) {
    state_->runner_was_created = true;
  }
  ~TestNativeRunner() override {
    state_->runner_was_destroyed = true;
    base::MessageLoop::current()->Quit();
  }
  void Start(const base::FilePath& app_path,
             mojo::InterfaceRequest<mojo::Application> application_request,
             const base::Closure& app_completed_callback) override {
    state_->runner_was_started = true;
  }

 private:
  TestState* state_;
};

class TestNativeRunnerFactory : public NativeRunnerFactory {
 public:
  explicit TestNativeRunnerFactory(TestState* state) : state_(state) {}
  ~TestNativeRunnerFactory() override {}
  scoped_ptr<NativeRunner> Create(
      const NativeApplicationOptions& /*options*/) override {
    return scoped_ptr<NativeRunner>(new TestNativeRunner(state_));
  }

 private:
  TestState* state_;
};

class NativeApplicationLoaderTest : public testing::Test,
                                    public ApplicationManager::Delegate {
 public:
  NativeApplicationLoaderTest()
      : application_manager_(ApplicationManager::Options(), this) {}
  ~NativeApplicationLoaderTest() override {}
  void SetUp() override {
    context_.Init();
    scoped_ptr<NativeRunnerFactory> factory(
        new TestNativeRunnerFactory(&state_));
    application_manager_.set_native_runner_factory(factory.Pass());
    application_manager_.set_blocking_pool(
        context_.task_runners()->blocking_pool());
  }
  void TearDown() override { context_.Shutdown(); }

 protected:
  shell::Context context_;
  base::MessageLoop loop_;
  ApplicationManager application_manager_;
  TestState state_;

 private:
  // ApplicationManager::Delegate
  GURL ResolveMappings(const GURL& url) override { return url; }
  GURL ResolveMojoURL(const GURL& url) override { return url; }
};

TEST_F(NativeApplicationLoaderTest, DoesNotExist) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath nonexistent_file(FILE_PATH_LITERAL("nonexistent.txt"));
  GURL url(FilePathToFileURL(temp_dir.path().Append(nonexistent_file)));
  application_manager_.ConnectToApplication(url, GURL(), nullptr,
                                            base::Closure());
  EXPECT_FALSE(state_.runner_was_created);
  EXPECT_FALSE(state_.runner_was_started);
  EXPECT_FALSE(state_.runner_was_destroyed);
}

}  // namespace
}  // namespace shell
