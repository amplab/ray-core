// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/application_test_base.h"

#include <utility>

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/environment/environment.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/interfaces/application/application.mojom.h"

namespace mojo {
namespace test {

namespace {

// Share the application command-line arguments with multiple application tests.
Array<String> g_args;

// Share the application URL with multiple application tests.
String g_url;

// Application request handle passed from the shell in MojoMain, stored in
// between SetUp()/TearDown() so we can (re-)intialize new ApplicationImpls.
InterfaceRequest<Application> g_application_request;

// Shell pointer passed in the initial mojo.Application.Initialize() call,
// stored in between initial setup and the first test and between SetUp/TearDown
// calls so we can (re-)initialize new ApplicationImpls.
ShellPtr g_shell;

void InitializeArgs(int argc, const char** argv) {
  MOJO_CHECK(g_args.is_null());
  for (int i = 0; i < argc; i++) {
    MOJO_CHECK(argv[i]);
    g_args.push_back(argv[i]);
  }
}

class ShellAndArgumentGrabber : public Application {
 public:
  ShellAndArgumentGrabber(Array<String>* args,
                          InterfaceRequest<Application> application_request)
      : args_(args), binding_(this, application_request.Pass()) {}

  void WaitForInitialize() {
    // Initialize is always the first call made on Application.
    MOJO_CHECK(binding_.WaitForIncomingMethodCall());
  }

 private:
  // Application implementation.
  void Initialize(InterfaceHandle<Shell> shell,
                  Array<String> args,
                  const mojo::String& url) override {
    *args_ = args.Pass();
    g_url = url;
    g_application_request = binding_.Unbind();
    g_shell = ShellPtr::Create(std::move(shell));
  }

  void AcceptConnection(const String& requestor_url,
                        const String& url,
                        InterfaceRequest<ServiceProvider> services) override {
    MOJO_CHECK(false);
  }

  void RequestQuit() override { MOJO_CHECK(false); }

  Array<String>* args_;
  Binding<Application> binding_;
};

}  // namespace

MojoResult RunAllTests(MojoHandle application_request_handle) {
  {
    // This loop is used for init, and then destroyed before running tests.
    Environment::InstantiateDefaultRunLoop();

    // Grab the shell handle and GTEST commandline arguments.
    // GTEST command line arguments are supported amid application arguments:
    // $ mojo_shell mojo:example_apptests
    //   --args-for='mojo:example_apptests arg1 --gtest_filter=foo arg2'
    Array<String> args;
    ShellAndArgumentGrabber grabber(
        &args, InterfaceRequest<Application>(MakeScopedHandle(
                   MessagePipeHandle(application_request_handle))));
    grabber.WaitForInitialize();
    MOJO_CHECK(g_shell);
    MOJO_CHECK(g_application_request.is_pending());

    // InitGoogleTest expects (argc + 1) elements, including a terminating null.
    // It also removes GTEST arguments from |argv| and updates the |argc| count.
    MOJO_CHECK(args.size() <
               static_cast<size_t>(std::numeric_limits<int>::max()));
    // We'll put our URL in at |argv[0]| (|args| doesn't include a "command
    // name").
    int argc = static_cast<int>(args.size()) + 1;
    std::vector<const char*> argv(argc + 1);
    argv[0] = g_url.get().c_str();
    for (size_t i = 0; i < args.size(); ++i)
      argv[i + 1] = args[i].get().c_str();
    argv[argc] = nullptr;

    // Note: |InitGoogleTest()| will modify |argc| and |argv[...]|.
    testing::InitGoogleTest(&argc, const_cast<char**>(&argv[0]));
    InitializeArgs(argc, &argv[0]);

    Environment::DestroyDefaultRunLoop();
  }

  int result = RUN_ALL_TESTS();

  // Shut down our message pipes before exiting.
  (void)g_application_request.PassMessagePipe();
  (void)g_shell.PassInterfaceHandle();

  return (result == 0) ? MOJO_RESULT_OK : MOJO_RESULT_UNKNOWN;
}

ApplicationTestBase::ApplicationTestBase() {}

ApplicationTestBase::~ApplicationTestBase() {}

void ApplicationTestBase::SetUp() {
  // A run loop is recommended for ApplicationImpl initialization and
  // communication.
  if (ShouldCreateDefaultRunLoop())
    Environment::InstantiateDefaultRunLoop();

  MOJO_CHECK(g_application_request.is_pending());
  MOJO_CHECK(g_shell);
  MOJO_CHECK(args_.empty());

  shell_ = g_shell.Pass();
  for (size_t i = 0; i < g_args.size(); i++)
    args_.push_back(g_args[i]);
}

void ApplicationTestBase::TearDown() {
  MOJO_CHECK(!g_shell);

  // TODO(vtl): The straightforward |g_shell = shell_.Pass();| causes tests to
  // hang. Presumably, it's because there are still requests to the shell
  // pending. :-(
  g_shell.Bind(shell_.PassInterfaceHandle());
  args_.clear();

  if (ShouldCreateDefaultRunLoop())
    Environment::DestroyDefaultRunLoop();
}

bool ApplicationTestBase::ShouldCreateDefaultRunLoop() {
  return true;
}

}  // namespace test
}  // namespace mojo
