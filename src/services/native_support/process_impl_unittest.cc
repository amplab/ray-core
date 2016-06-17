// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include <memory>
#include <string>
#include <vector>

#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/files/cpp/output_stream_file.h"
#include "mojo/services/files/interfaces/file.mojom.h"
#include "mojo/services/files/interfaces/types.mojom.h"
// FIXME
#include "mojo/services/native_support/interfaces/process.mojom.h"
#include "services/native_support/process_test_base.h"

using mojo::SynchronousInterfacePtr;

namespace native_support {
namespace {

using ProcessImplTest = ProcessTestBase;

mojo::Array<uint8_t> ToByteArray(const std::string& s) {
  auto rv = mojo::Array<uint8_t>::New(s.size());
  memcpy(rv.data(), s.data(), s.size());
  return rv;
}

// This also (slightly) tests |Wait()|, since we want to have some evidence that
// we ran the specified binary (/bin/true versus /bin/false).
TEST_F(ProcessImplTest, Spawn) {
  mojo::files::Error error;

  {
    SynchronousInterfacePtr<ProcessController> process_controller;
    error = mojo::files::Error::INTERNAL;
    ASSERT_TRUE(process()->Spawn(
        ToByteArray("/bin/true"), nullptr, nullptr, nullptr, nullptr, nullptr,
        GetSynchronousProxy(&process_controller), &error));
    EXPECT_EQ(mojo::files::Error::OK, error);

    error = mojo::files::Error::INTERNAL;
    int32_t exit_status = 42;
    ASSERT_TRUE(process_controller->Wait(&error, &exit_status));
    EXPECT_EQ(mojo::files::Error::OK, error);
    EXPECT_EQ(0, exit_status);
  }

  {
    SynchronousInterfacePtr<ProcessController> process_controller;
    error = mojo::files::Error::INTERNAL;
    ASSERT_TRUE(process()->Spawn(
        ToByteArray("/bin/false"), nullptr, nullptr, nullptr, nullptr, nullptr,
        GetSynchronousProxy(&process_controller), &error));
    EXPECT_EQ(mojo::files::Error::OK, error);

    error = mojo::files::Error::INTERNAL;
    int32_t exit_status = 0;
    ASSERT_TRUE(process_controller->Wait(&error, &exit_status));
    EXPECT_EQ(mojo::files::Error::OK, error);
    EXPECT_NE(exit_status, 0);
  }
}

void QuitMessageLoop() {
  base::MessageLoop::current()->QuitWhenIdle();
}

void RunMessageLoop() {
  base::MessageLoop::current()->Run();
}

class CaptureOutputFile : public files_impl::OutputStreamFile::Client {
 public:
  explicit CaptureOutputFile(mojo::InterfaceRequest<mojo::files::File> request)
      : impl_(files_impl::OutputStreamFile::Create(this, request.Pass())) {}
  ~CaptureOutputFile() override {}

  const std::string& output() const { return output_; }
  bool is_closed() const { return is_closed_; }

 private:
  // |files_impl::OutputStreamFile::Client|:
  void OnDataReceived(const void* bytes, size_t num_bytes) override {
    output_.append(static_cast<const char*>(bytes), num_bytes);
    QuitMessageLoop();
  }
  void OnClosed() override {
    is_closed_ = true;
    QuitMessageLoop();
  }

  std::string output_;
  bool is_closed_ = false;
  std::unique_ptr<files_impl::OutputStreamFile> impl_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(CaptureOutputFile);
};

// Spawn a native binary and redirect its stdout to a Mojo "file" that captures
// it.
TEST_F(ProcessImplTest, SpawnRedirectStdout) {
  static const char kOutput[] = "hello mojo!";

  mojo::files::FilePtr file;
  CaptureOutputFile file_impl(GetProxy(&file));

  mojo::Array<mojo::Array<uint8_t>> argv;
  argv.push_back(ToByteArray("/bin/echo"));
  argv.push_back(ToByteArray(kOutput));
  SynchronousInterfacePtr<ProcessController> process_controller;
  mojo::files::Error error = mojo::files::Error::INTERNAL;
  ASSERT_TRUE(process()->Spawn(
      ToByteArray("/bin/echo"), argv.Pass(), nullptr, nullptr, file.Pass(),
      nullptr, GetSynchronousProxy(&process_controller), &error));
  EXPECT_EQ(mojo::files::Error::OK, error);

  // Since |file|'s impl is on our thread, we have to spin our message loop.
  RunMessageLoop();
  // /bin/echo adds a newline (alas, POSIX /bin/echo doesn't specify "-n").
  EXPECT_EQ(std::string(kOutput) + "\n", file_impl.output());

  error = mojo::files::Error::INTERNAL;
  int32_t exit_status = 0;
  ASSERT_TRUE(process_controller->Wait(&error, &exit_status));
  EXPECT_EQ(mojo::files::Error::OK, error);
  EXPECT_EQ(0, exit_status);

  // TODO(vtl): Currently, |file| won't be closed until the process controller
  // is closed, even if the child has been waited on and all output read.
  // Possibly this should be changed, but we currently can't since the I/O
  // thread doesn't have facilities for informing us that an FD will never be
  // readable.
  EXPECT_FALSE(file_impl.is_closed());
  process_controller.reset();
  RunMessageLoop();
  EXPECT_TRUE(file_impl.is_closed());
}

}  // namespace
}  // namespace native_support
