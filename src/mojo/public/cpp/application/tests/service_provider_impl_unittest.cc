// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/service_provider_impl.h"

#include <utility>

#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/public/interfaces/bindings/tests/ping_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

class ServiceProviderImplTest : public testing::Test {
 public:
  ServiceProviderImplTest() {}
  ~ServiceProviderImplTest() override { loop_.RunUntilIdle(); }

  RunLoop& loop() { return loop_; }

 protected:
  void QuitLoop(bool ok) {
    EXPECT_TRUE(ok);
    loop_.Quit();
  }

 private:
  RunLoop loop_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ServiceProviderImplTest);
};

class PingServiceImpl : public test::PingService {
 public:
  PingServiceImpl(InterfaceRequest<test::PingService> ping_service_request)
      : strong_binding_(this, std::move(ping_service_request)) {}
  ~PingServiceImpl() override {}

  // |test::PingService|:
  void Ping(const PingCallback& callback) override { callback.Run(); }

 private:
  StrongBinding<test::PingService> strong_binding_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(PingServiceImpl);
};

TEST_F(ServiceProviderImplTest, Basic) {
  const char kRemoteUrl[] = "https://example.com/remote.mojo";
  const char kConnectionUrl[] = "https://example.com/me.mojo";

  const char kPing1[] = "Ping1";
  const char kPing2[] = "Ping2";
  const char kPing3[] = "Ping3";

  ServiceProviderPtr sp;
  ServiceProviderImpl impl(ConnectionContext(ConnectionContext::Type::INCOMING,
                                             kRemoteUrl, kConnectionUrl),
                           GetProxy(&sp));
  EXPECT_EQ(ConnectionContext::Type::INCOMING, impl.connection_context().type);
  EXPECT_EQ(kRemoteUrl, impl.connection_context().remote_url);
  EXPECT_EQ(kConnectionUrl, impl.connection_context().connection_url);

  impl.AddService<test::PingService>(
      [&kRemoteUrl, &kConnectionUrl](
          const ConnectionContext& connection_context,
          InterfaceRequest<test::PingService> ping_service_request) {
        EXPECT_EQ(ConnectionContext::Type::INCOMING, connection_context.type);
        EXPECT_EQ(kRemoteUrl, connection_context.remote_url);
        EXPECT_EQ(kConnectionUrl, connection_context.connection_url);
        new PingServiceImpl(std::move(ping_service_request));
      },
      kPing1);

  impl.AddService<test::PingService>(
      [&kRemoteUrl, &kConnectionUrl](
          const ConnectionContext& connection_context,
          InterfaceRequest<test::PingService> ping_service_request) {
        EXPECT_EQ(ConnectionContext::Type::INCOMING, connection_context.type);
        EXPECT_EQ(kRemoteUrl, connection_context.remote_url);
        EXPECT_EQ(kConnectionUrl, connection_context.connection_url);
        new PingServiceImpl(std::move(ping_service_request));
      },
      kPing2);

  {
    test::PingServicePtr ping1;
    ConnectToService(sp.get(), GetProxy(&ping1), kPing1);
    ping1.set_connection_error_handler([this] { QuitLoop(false); });
    ping1->Ping([this] { QuitLoop(true); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  {
    test::PingServicePtr ping2;
    ConnectToService(sp.get(), GetProxy(&ping2), kPing2);
    ping2.set_connection_error_handler([this] { QuitLoop(false); });
    ping2->Ping([this] { QuitLoop(true); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  // "Ping3" isn't actually registered!
  {
    test::PingServicePtr ping3;
    ConnectToService(sp.get(), GetProxy(&ping3), kPing3);
    ping3.set_connection_error_handler([this] { QuitLoop(true); });
    ping3->Ping([this] { QuitLoop(false); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  impl.RemoveService<test::PingService>(kPing2);

  // "Ping2" should no longer work.
  {
    test::PingServicePtr ping2;
    ConnectToService(sp.get(), GetProxy(&ping2), kPing2);
    ping2.set_connection_error_handler([this] { QuitLoop(true); });
    ping2->Ping([this] { QuitLoop(false); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  // But "Ping1" should still work.
  {
    test::PingServicePtr ping1;
    ConnectToService(sp.get(), GetProxy(&ping1), kPing1);
    ping1.set_connection_error_handler([this] { QuitLoop(false); });
    ping1->Ping([this] { QuitLoop(true); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  impl.RemoveServiceForName(kPing1);

  // "Ping1" should no longer work.
  {
    test::PingServicePtr ping1;
    ConnectToService(sp.get(), GetProxy(&ping1), kPing1);
    ping1.set_connection_error_handler([this] { QuitLoop(true); });
    ping1->Ping([this] { QuitLoop(false); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  sp.reset();
  loop().RunUntilIdle();
}

TEST_F(ServiceProviderImplTest, CloseAndRebind) {
  const char kRemoteUrl1[] = "https://example.com/remote1.mojo";
  const char kRemoteUrl2[] = "https://example.com/remote2.mojo";
  const char kConnectionUrl[] = "https://example.com/me.mojo";
  const char kPing[] = "Ping";

  ServiceProviderPtr sp1;
  ServiceProviderImpl impl(ConnectionContext(ConnectionContext::Type::INCOMING,
                                             kRemoteUrl1, kConnectionUrl),
                           GetProxy(&sp1));
  EXPECT_EQ(ConnectionContext::Type::INCOMING, impl.connection_context().type);
  EXPECT_EQ(kRemoteUrl1, impl.connection_context().remote_url);
  EXPECT_EQ(kConnectionUrl, impl.connection_context().connection_url);

  impl.AddService<test::PingService>(
      [&kRemoteUrl1, &kRemoteUrl2, &kConnectionUrl](
          const ConnectionContext& connection_context,
          InterfaceRequest<test::PingService> ping_service_request) {
        EXPECT_EQ(ConnectionContext::Type::INCOMING, connection_context.type);
        EXPECT_TRUE(connection_context.remote_url == kRemoteUrl1 ||
                    connection_context.remote_url == kRemoteUrl2);
        EXPECT_EQ(kConnectionUrl, connection_context.connection_url);
        new PingServiceImpl(std::move(ping_service_request));
      },
      kPing);

  {
    test::PingServicePtr ping;
    ConnectToService(sp1.get(), GetProxy(&ping), kPing);
    ping.set_connection_error_handler([this] { QuitLoop(false); });
    ping->Ping([this] { QuitLoop(true); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  impl.Close();
  EXPECT_EQ(ConnectionContext::Type::UNKNOWN, impl.connection_context().type);
  EXPECT_EQ(std::string(), impl.connection_context().remote_url);
  EXPECT_EQ(std::string(), impl.connection_context().connection_url);
  sp1.reset();
  loop().RunUntilIdle();

  ServiceProviderPtr sp2;
  impl.Bind(ConnectionContext(ConnectionContext::Type::INCOMING, kRemoteUrl2,
                              kConnectionUrl),
            GetProxy(&sp2));
  EXPECT_EQ(ConnectionContext::Type::INCOMING, impl.connection_context().type);
  EXPECT_EQ(kRemoteUrl2, impl.connection_context().remote_url);
  EXPECT_EQ(kConnectionUrl, impl.connection_context().connection_url);

  {
    test::PingServicePtr ping;
    ConnectToService(sp2.get(), GetProxy(&ping), kPing);
    ping.set_connection_error_handler([this] { QuitLoop(false); });
    ping->Ping([this] { QuitLoop(true); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  // Can close multiple times.
  impl.Close();
  impl.Close();
  sp2.reset();
  loop().RunUntilIdle();
}

TEST_F(ServiceProviderImplTest, Bind) {
  const char kRemoteUrl[] = "https://example.com/remote.mojo";
  const char kConnectionUrl[] = "https://example.com/me.mojo";
  const char kPing[] = "Ping";

  ServiceProviderPtr sp;
  ServiceProviderImpl impl;
  EXPECT_EQ(ConnectionContext::Type::UNKNOWN, impl.connection_context().type);
  EXPECT_EQ(std::string(), impl.connection_context().remote_url);
  EXPECT_EQ(std::string(), impl.connection_context().connection_url);

  impl.Bind(ConnectionContext(ConnectionContext::Type::INCOMING, kRemoteUrl,
                              kConnectionUrl),
            GetProxy(&sp));

  impl.AddService<test::PingService>(
      [&kRemoteUrl, &kConnectionUrl](
          const ConnectionContext& connection_context,
          InterfaceRequest<test::PingService> request) {
        EXPECT_EQ(ConnectionContext::Type::INCOMING, connection_context.type);
        EXPECT_EQ(kRemoteUrl, connection_context.remote_url);
        EXPECT_EQ(kConnectionUrl, connection_context.connection_url);
        new PingServiceImpl(std::move(request));
      },
      kPing);

  {
    test::PingServicePtr ping;
    ConnectToService(sp.get(), GetProxy(&ping), kPing);
    ping.set_connection_error_handler([this] { QuitLoop(false); });
    ping->Ping([this] { QuitLoop(true); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  sp.reset();
  loop().RunUntilIdle();
}

class FauxServiceProvider : public ServiceProvider {
 public:
  explicit FauxServiceProvider(
      std::function<void(const std::string& service_name)>
          on_connect_to_service)
      : on_connect_to_service_(on_connect_to_service) {}
  ~FauxServiceProvider() override {}

  // |ServiceProvider|:
  void ConnectToService(const String& service_name,
                        ScopedMessagePipeHandle client_handle) override {
    on_connect_to_service_(service_name.get());
  }

 private:
  std::function<void(const std::string& service_name)> on_connect_to_service_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(FauxServiceProvider);
};

TEST_F(ServiceProviderImplTest, FallbackServiceProvider) {
  const char kWhatever[] = "Whatever";

  ServiceProviderPtr sp;
  ServiceProviderImpl impl(ConnectionContext(ConnectionContext::Type::INCOMING,
                                             "https://example.com/remote.mojo",
                                             "https://example.com/me.mojo"),
                           GetProxy(&sp));

  {
    test::PingServicePtr ping;
    ConnectToService(sp.get(), GetProxy(&ping), kWhatever);
    ping.set_connection_error_handler([this] { QuitLoop(true); });
    ping->Ping([this] { QuitLoop(false); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  bool was_run = false;
  FauxServiceProvider fallback_sp(
      [this, &kWhatever, &was_run](const std::string& service_name) {
        EXPECT_EQ(kWhatever, service_name);
        was_run = true;
      });
  impl.set_fallback_service_provider(&fallback_sp);

  {
    test::PingServicePtr ping;
    ConnectToService(sp.get(), GetProxy(&ping), kWhatever);
    ping.set_connection_error_handler([this] { QuitLoop(true); });
    EXPECT_FALSE(was_run);
    ping->Ping([this] { QuitLoop(false); });
    loop().Run();
    EXPECT_TRUE(was_run);
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  // Clear the fallback.
  impl.set_fallback_service_provider(nullptr);
  was_run = false;

  {
    test::PingServicePtr ping;
    ConnectToService(sp.get(), GetProxy(&ping), kWhatever);
    ping.set_connection_error_handler([this] { QuitLoop(true); });
    ping->Ping([this] { QuitLoop(false); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  sp.reset();
  loop().RunUntilIdle();

  EXPECT_FALSE(was_run);
}

TEST_F(ServiceProviderImplTest, ConstructRequestNotPending) {
  ServiceProviderImpl impl(ConnectionContext(ConnectionContext::Type::INCOMING,
                                             "https://example.com/remote.mojo",
                                             "https://example.com/me.mojo"),
                           InterfaceRequest<ServiceProvider>());
  EXPECT_EQ(ConnectionContext::Type::UNKNOWN, impl.connection_context().type);
  EXPECT_EQ(std::string(), impl.connection_context().remote_url);
  EXPECT_EQ(std::string(), impl.connection_context().connection_url);
}

// TODO(vtl): Explicitly test |AddServiceForName()|?

}  // namespace
}  // namespace mojo
