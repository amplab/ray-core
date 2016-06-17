// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "shell/crash/crash_upload.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace breakpad {
namespace {

class MockURLLoader : public mojo::URLLoader {
 public:
  MockURLLoader() : called_(false) {}
  ~MockURLLoader() override {}

  bool called() const { return called_; }

  std::string GetLastContent() {
    std::string result;
    mojo::common::BlockingCopyToString(body_.Pass(), &result);
    return result;
  }

 private:
  // Overriden from mojo::NetworkService.
  void Start(mojo::URLRequestPtr request,
             const StartCallback& callback) override {
    called_ = true;
    mojo::URLResponsePtr response = mojo::URLResponse::New();
    response->status_code = 200;
    body_ = request->body[0].Pass();
    callback.Run(response.Pass());
  }
  void FollowRedirect(const FollowRedirectCallback& callback) override {}
  void QueryStatus(const QueryStatusCallback& callback) override {}

  bool called_;
  mojo::ScopedDataPipeConsumerHandle body_;
};

class MockNetworkService : public mojo::NetworkService {
 public:
  MockNetworkService() : binding_(&mock_url_loader_) {}
  ~MockNetworkService() override {}

  MockURLLoader* url_loader() { return &mock_url_loader_; }

 private:
  // Overriden from mojo::NetworkService.
  void CreateURLLoader(
      mojo::InterfaceRequest<mojo::URLLoader> loader) override {
    binding_.Bind(loader.Pass());
  }
  void GetCookieStore(
      mojo::InterfaceRequest<mojo::CookieStore> cookie_store) override {}
  void CreateWebSocket(
      mojo::InterfaceRequest<mojo::WebSocket> socket) override {}
  void CreateTCPBoundSocket(
      mojo::NetAddressPtr local_address,
      mojo::InterfaceRequest<mojo::TCPBoundSocket> bound_socket,
      const CreateTCPBoundSocketCallback& callback) override {}
  void CreateTCPConnectedSocket(
      mojo::NetAddressPtr remote_address,
      mojo::ScopedDataPipeConsumerHandle send_stream,
      mojo::ScopedDataPipeProducerHandle receive_stream,
      mojo::InterfaceRequest<mojo::TCPConnectedSocket> client_socket,
      const CreateTCPConnectedSocketCallback& callback) override {}
  void CreateUDPSocket(
      mojo::InterfaceRequest<mojo::UDPSocket> socket) override {}
  void CreateHttpServer(
      mojo::NetAddressPtr local_address,
      mojo::InterfaceHandle<mojo::HttpServerDelegate> delegate,
      const CreateHttpServerCallback& callback) override {}
  void RegisterURLLoaderInterceptor(
      mojo::InterfaceHandle<mojo::URLLoaderInterceptorFactory> factory)
      override {}
  void CreateHostResolver(
      mojo::InterfaceRequest<mojo::HostResolver> host_resolver) override {}

  MockURLLoader mock_url_loader_;
  mojo::Binding<mojo::URLLoader> binding_;
};

std::string GetDumpContent(const base::FilePath& path) {
  return "--" + path.BaseName().value() + "\r\n";
}

void CreateValidDump(const base::FilePath& path, const base::Time& time) {
  std::string content = GetDumpContent(path);
  base::WriteFile(path, content.data(), content.size());
  base::TouchFile(path, time, time);
}

int CountFiles(const base::FilePath& path) {
  base::FileEnumerator files(path, false, base::FileEnumerator::FILES);
  int result = 0;
  for (base::FilePath file = files.Next(); !file.empty(); file = files.Next())
    ++result;
  return result;
}

base::FilePath GetSentinelPath(const base::FilePath& path) {
  return path.Append("upload.sentinel");
}

class CrashUploadTest : public testing::Test {
 public:
  CrashUploadTest()
      : loop_(mojo::common::MessagePumpMojo::Create()),
        binding_(&mock_network_service_, GetProxy(&network_service_)) {}

  void SetUp() override { ASSERT_TRUE(dumps_dir_.CreateUniqueTempDir()); }

  void RebindNetworkService() {
    binding_.Close();
    binding_.Bind(GetProxy(&network_service_));
  }

 protected:
  base::ShadowingAtExitManager at_exit_;
  base::MessageLoop loop_;
  MockNetworkService mock_network_service_;
  mojo::NetworkServicePtr network_service_;
  mojo::Binding<mojo::NetworkService> binding_;
  base::ScopedTempDir dumps_dir_;
};

// Tests that reports can be uploaded and that only the most recent report is
// uploaded.
TEST_F(CrashUploadTest, UploadToServer) {
  for (int i = 0; i < 3; ++i) {
    CreateValidDump(dumps_dir_.path().Append(base::StringPrintf("%d.dmp", i)),
                    base::Time::Now() - base::TimeDelta::FromHours(i));
  }

  UploadCrashes(dumps_dir_.path(), loop_.task_runner().get(),
                network_service_.Pass());
  loop_.PostTask(FROM_HERE, base::MessageLoop::QuitClosure());
  base::RunLoop().Run();

  EXPECT_TRUE(mock_network_service_.url_loader()->called());
  EXPECT_TRUE(base::PathExists(GetSentinelPath(dumps_dir_.path())));
  EXPECT_EQ(GetDumpContent(dumps_dir_.path().Append("0.dmp")),
            mock_network_service_.url_loader()->GetLastContent());
  EXPECT_EQ(1, CountFiles(dumps_dir_.path()));
}

// Tests that nothing happen when there is nothing to upload.
TEST_F(CrashUploadTest, NothingToUpload) {
  UploadCrashes(dumps_dir_.path(), loop_.task_runner().get(),
                network_service_.Pass());
  loop_.PostTask(FROM_HERE, base::MessageLoop::QuitClosure());
  base::RunLoop().Run();

  EXPECT_FALSE(mock_network_service_.url_loader()->called());
  EXPECT_EQ(0, CountFiles(dumps_dir_.path()));
}

// Tests that spurious reports are deleted but none are uploaded when there is a
// recent sentinel file. Check that the report is uploaded when the sentinel is
// old enough.
TEST_F(CrashUploadTest, UploadGuardedBySentinel) {
  for (int i = 0; i < 3; ++i) {
    CreateValidDump(dumps_dir_.path().Append(base::StringPrintf("%d.dmp", i)),
                    base::Time::Now() - base::TimeDelta::FromHours(i));
  }
  base::FilePath sentinel = GetSentinelPath(dumps_dir_.path());
  base::WriteFile(sentinel, nullptr, 0);
  base::TouchFile(sentinel,
                  base::Time::Now() - base::TimeDelta::FromMinutes(30),
                  base::Time::Now() - base::TimeDelta::FromMinutes(30));

  UploadCrashes(dumps_dir_.path(), loop_.task_runner().get(),
                network_service_.Pass());
  loop_.PostTask(FROM_HERE, base::MessageLoop::QuitClosure());
  base::RunLoop().Run();

  EXPECT_FALSE(mock_network_service_.url_loader()->called());
  EXPECT_TRUE(base::PathExists(GetSentinelPath(dumps_dir_.path())));
  EXPECT_EQ(2, CountFiles(dumps_dir_.path()));

  // Send the sentinel in the past.
  base::TouchFile(sentinel,
                  base::Time::Now() - base::TimeDelta::FromMinutes(90),
                  base::Time::Now() - base::TimeDelta::FromMinutes(90));

  RebindNetworkService();
  UploadCrashes(dumps_dir_.path(), loop_.task_runner().get(),
                network_service_.Pass());
  loop_.PostTask(FROM_HERE, base::MessageLoop::QuitClosure());
  base::RunLoop().Run();

  EXPECT_TRUE(mock_network_service_.url_loader()->called());
  EXPECT_TRUE(base::PathExists(GetSentinelPath(dumps_dir_.path())));
  EXPECT_EQ(GetDumpContent(dumps_dir_.path().Append("0.dmp")),
            mock_network_service_.url_loader()->GetLastContent());
  EXPECT_EQ(1, CountFiles(dumps_dir_.path()));
}

}  // namespace
}  // namespace breakpad
