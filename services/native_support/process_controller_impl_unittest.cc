// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <signal.h>
#include <string.h>

#include <string>
#include <vector>

#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "mojo/services/files/cpp/input_stream_file.h"
#include "mojo/services/files/cpp/output_stream_file.h"
#include "mojo/services/files/interfaces/types.mojom.h"
#include "services/native_support/process_test_base.h"

using mojo::SynchronousInterfacePtr;

namespace native_support {
namespace {

using ProcessControllerImplTest = ProcessTestBase;

mojo::Array<uint8_t> ToByteArray(const std::string& s) {
  auto rv = mojo::Array<uint8_t>::New(s.size());
  memcpy(rv.data(), s.data(), s.size());
  return rv;
}

void QuitMessageLoop() {
  base::MessageLoop::current()->QuitWhenIdle();
}

void RunMessageLoop() {
  base::MessageLoop::current()->Run();
}

// Note: We already tested success (zero) versus failure (non-zero) exit
// statuses in |ProcessControllerTest.Spawn|.
TEST_F(ProcessControllerImplTest, Wait) {
  mojo::files::Error error;

  {
    SynchronousInterfacePtr<ProcessController> process_controller;
    error = mojo::files::Error::INTERNAL;
    const char kPath[] = "/bin/sh";
    mojo::Array<mojo::Array<uint8_t>> argv;
    argv.push_back(ToByteArray(kPath));
    argv.push_back(ToByteArray("-c"));
    argv.push_back(ToByteArray("exit 42"));
    ASSERT_TRUE(process()->Spawn(
        ToByteArray(kPath), argv.Pass(), nullptr, nullptr, nullptr, nullptr,
        GetSynchronousProxy(&process_controller), &error));
    EXPECT_EQ(mojo::files::Error::OK, error);

    error = mojo::files::Error::INTERNAL;
    int32_t exit_status = 0;
    ASSERT_TRUE(process_controller->Wait(&error, &exit_status));
    EXPECT_EQ(mojo::files::Error::OK, error);
    EXPECT_EQ(42, exit_status);
  }
}

// An output file stream that captures output and quits when "ready" is seen.
class QuitOnReadyFile : public files_impl::OutputStreamFile::Client {
 public:
  explicit QuitOnReadyFile(mojo::InterfaceRequest<mojo::files::File> request)
      : impl_(files_impl::OutputStreamFile::Create(this, request.Pass())) {}
  ~QuitOnReadyFile() override {}

  bool got_ready() const { return got_ready_; }

 private:
  // |files_impl::OutputStreamFile::Client|:
  void OnDataReceived(const void* bytes, size_t num_bytes) override {
    output_.append(static_cast<const char*>(bytes), num_bytes);
    if (output_.find("ready") != std::string::npos) {
      got_ready_ = true;
      QuitMessageLoop();
    }
  }
  void OnClosed() override { QuitMessageLoop(); }

  std::string output_;
  bool got_ready_ = false;
  std::unique_ptr<files_impl::OutputStreamFile> impl_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(QuitOnReadyFile);
};

// An input file stream that ignores reads (never completing them).
class IgnoreReadsFile : public files_impl::InputStreamFile::Client {
 public:
  explicit IgnoreReadsFile(mojo::InterfaceRequest<mojo::files::File> request)
      : impl_(files_impl::InputStreamFile::Create(this, request.Pass())) {}
  ~IgnoreReadsFile() override {}

 private:
  // |files_impl::InputStreamFile::Client|:
  bool RequestData(size_t max_num_bytes,
                   mojo::files::Error* error,
                   mojo::Array<uint8_t>* data,
                   const RequestDataCallback& callback) override {
    // Don't let |callback| die, because we probably have assertions "ensuring"
    // that response callbacks get called.
    callbacks_.push_back(callback);
    return false;
  }
  void OnClosed() override { QuitMessageLoop(); }

  std::vector<RequestDataCallback> callbacks_;
  std::unique_ptr<files_impl::InputStreamFile> impl_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(IgnoreReadsFile);
};

TEST_F(ProcessControllerImplTest, Kill) {
  // We want to run bash and have it set a trap for SIGINT, which it'll handle
  // by quitting with code 42. We need to make sure that bash started and set
  // the trap before we kill our child. Thus we have bash echo "ready" and wait
  // for it to do so (and thus we need to capture/examine stdout).
  //
  // We want bash to pause, so that we can kill it before it quits. We can't use
  // "sleep" since it's not a builtin (so bash would wait for it before dying).
  // So instead we use "read" with a timeout (note that "-t" is a bash
  // extension, and not specified by POSIX for /bin/sh). Thus we have to give
  // something for stdin (otherwise, it'd come from /dev/null and the read would
  // be completed immediately).
  mojo::files::FilePtr ifile;
  IgnoreReadsFile ifile_impl(GetProxy(&ifile));
  mojo::files::FilePtr ofile;
  QuitOnReadyFile ofile_impl(GetProxy(&ofile));

  SynchronousInterfacePtr<ProcessController> process_controller;
  mojo::files::Error error = mojo::files::Error::INTERNAL;
  const char kPath[] = "/bin/bash";
  mojo::Array<mojo::Array<uint8_t>> argv;
  argv.push_back(ToByteArray(kPath));
  argv.push_back(ToByteArray("-c"));
  argv.push_back(
      ToByteArray("trap 'exit 42' INT; echo ready; read -t30; exit 1"));
  ASSERT_TRUE(process()->Spawn(
      ToByteArray(kPath), argv.Pass(), nullptr, ifile.Pass(), ofile.Pass(),
      nullptr, GetSynchronousProxy(&process_controller), &error));
  EXPECT_EQ(mojo::files::Error::OK, error);

  // |ofile_impl| will quit the message loop once it sees "ready".
  RunMessageLoop();
  ASSERT_TRUE(ofile_impl.got_ready());

  // Send SIGINT.
  error = mojo::files::Error::INTERNAL;
  ASSERT_TRUE(process_controller->Kill(static_cast<int32_t>(SIGINT), &error));
  EXPECT_EQ(mojo::files::Error::OK, error);

  error = mojo::files::Error::INTERNAL;
  int32_t exit_status = 0;
  ASSERT_TRUE(process_controller->Wait(&error, &exit_status));
  EXPECT_EQ(mojo::files::Error::OK, error);
  EXPECT_EQ(42, exit_status);
}

TEST_F(ProcessControllerImplTest, DestroyingControllerKills) {
  // We want to make sure that we've exec-ed before killing, so we do what we do
  // in |ProcessControllerImplTest.Kill| (without the trap).
  {
    mojo::files::FilePtr ifile;
    IgnoreReadsFile ifile_impl(GetProxy(&ifile));
    mojo::files::FilePtr ofile;
    QuitOnReadyFile ofile_impl(GetProxy(&ofile));

    SynchronousInterfacePtr<ProcessController> process_controller;
    mojo::files::Error error = mojo::files::Error::INTERNAL;
    const char kPath[] = "/bin/bash";
    mojo::Array<mojo::Array<uint8_t>> argv;
    argv.push_back(ToByteArray(kPath));
    argv.push_back(ToByteArray("-c"));
    argv.push_back(ToByteArray("echo ready; read -t30"));
    ASSERT_TRUE(process()->Spawn(
        ToByteArray(kPath), argv.Pass(), nullptr, ifile.Pass(), ofile.Pass(),
        nullptr, GetSynchronousProxy(&process_controller), &error));
    EXPECT_EQ(mojo::files::Error::OK, error);

    // |ofile_impl| will quit the message loop once it sees "ready".
    RunMessageLoop();
    ASSERT_TRUE(ofile_impl.got_ready());
  }

  // The child should be killed.
  // TODO(vtl): It's pretty hard to verify that the child process was actually
  // killed. This could be done, e.g., by having the child trap SIGTERM and
  // writing something to a file, and then separately checking for that file.
  // For now now, just be happy if it doesn't crash. (I've actually verified it
  // "manually", but automation is hard.)
}

}  // namespace
}  // namespace native_support
