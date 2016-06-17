// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "mojo/converters/base/base_type_converters.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/interfaces/application/application_connector.mojom.h"
#include "mojo/services/http_server/cpp/http_server_util.h"
#include "mojo/services/http_server/interfaces/http_server.mojom-sync.h"
#include "mojo/services/http_server/interfaces/http_server_factory.mojom.h"
#include "mojo/services/network/interfaces/net_address.mojom.h"
#include "shell/kPingable.h"
#include "shell/test/pingable.mojom.h"

using mojo::String;

namespace {

std::string GetURL(uint16_t port, const std::string& path) {
  return base::StringPrintf("http://127.0.0.1:%u/%s",
                            static_cast<unsigned>(port), path.c_str());
}

class GetHandler : public http_server::HttpHandler {
 public:
  GetHandler(mojo::InterfaceRequest<http_server::HttpHandler> request,
             uint16_t port)
      : binding_(this, request.Pass()), port_(port) {}
  ~GetHandler() override {}

 private:
  // http_server::HttpHandler:
  void HandleRequest(http_server::HttpRequestPtr request,
                     const mojo::Callback<void(http_server::HttpResponsePtr)>&
                         callback) override {
    http_server::HttpResponsePtr response;
    if (base::StartsWith(request->relative_url.To<base::StringPiece>(), "/app",
                         base::CompareCase::SENSITIVE)) {
      response = http_server::CreateHttpResponse(
          200, std::string(shell::test::kPingable.data,
                           shell::test::kPingable.size));
      response->content_type = "application/octet-stream";
    } else if (request->relative_url == "/redirect") {
      response = http_server::HttpResponse::New();
      response->status_code = 302;
      response->custom_headers.insert("Location", GetURL(port_, "app"));
    } else {
      NOTREACHED();
    }

    callback.Run(response.Pass());
  }

  mojo::Binding<http_server::HttpHandler> binding_;
  uint16_t port_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(GetHandler);
};

typedef mojo::test::ApplicationTestBase ShellAppTest;

class ShellHTTPAppTest : public ShellAppTest {
 public:
  ShellHTTPAppTest() {}
  ~ShellHTTPAppTest() override {}

 protected:
  void SetUp() override {
    ShellAppTest::SetUp();

    mojo::ConnectToService(shell(), "mojo:http_server",
                           GetProxy(&http_server_factory_));

    mojo::NetAddressPtr local_address(mojo::NetAddress::New());
    local_address->family = mojo::NetAddressFamily::IPV4;
    local_address->ipv4 = mojo::NetAddressIPv4::New();
    local_address->ipv4->addr.resize(4);
    local_address->ipv4->addr[0] = 127;
    local_address->ipv4->addr[1] = 0;
    local_address->ipv4->addr[2] = 0;
    local_address->ipv4->addr[3] = 1;
    local_address->ipv4->port = 0;
    http_server_factory_->CreateHttpServer(GetSynchronousProxy(&http_server_),
                                           local_address.Pass());

    EXPECT_TRUE(http_server_->GetPort(&port_));

    http_server::HttpHandlerPtr http_handler;
    handler_.reset(new GetHandler(GetProxy(&http_handler).Pass(), port_));
    bool result = false;
    EXPECT_TRUE(http_server_->SetHandler(".*", http_handler.Pass(), &result));
    EXPECT_TRUE(result);
  }

  std::string GetURL(const std::string& path) { return ::GetURL(port_, path); }

  http_server::HttpServerFactoryPtr http_server_factory_;
  mojo::SynchronousInterfacePtr<http_server::HttpServer> http_server_;
  scoped_ptr<GetHandler> handler_;
  uint16_t port_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(ShellHTTPAppTest);
};

// Test that we can load apps over http.
TEST_F(ShellHTTPAppTest, Http) {
  PingablePtr pingable;
  mojo::ConnectToService(shell(), GetURL("app"), GetProxy(&pingable));
  pingable->Ping("hello",
                 [this](const String& app_url, const String& connection_url,
                        const String& message) {
                   EXPECT_EQ(GetURL("app"), app_url);
                   EXPECT_EQ(GetURL("app"), connection_url);
                   EXPECT_EQ("hello", message);
                   base::MessageLoop::current()->Quit();
                 });
  base::RunLoop().Run();
}

// Test that redirects work.
// TODO(aa): Test that apps receive the correct URL parameters.
TEST_F(ShellHTTPAppTest, Redirect) {
  PingablePtr pingable;
  mojo::ConnectToService(shell(), GetURL("redirect"), GetProxy(&pingable));
  pingable->Ping("hello",
                 [this](const String& app_url, const String& connection_url,
                        const String& message) {
                   EXPECT_EQ(GetURL("app"), app_url);
                   EXPECT_EQ(GetURL("app"), connection_url);
                   EXPECT_EQ("hello", message);
                   base::MessageLoop::current()->Quit();
                 });
  base::RunLoop().Run();
}

// Test that querystring is not considered when resolving http applications.
// TODO(aa|qsr): Fix this test on Linux ASAN http://crbug.com/463662
#if defined(ADDRESS_SANITIZER)
#define MAYBE_QueryHandling DISABLED_QueryHandling
#else
#define MAYBE_QueryHandling QueryHandling
#endif  // ADDRESS_SANITIZER
TEST_F(ShellHTTPAppTest, MAYBE_QueryHandling) {
  PingablePtr pingable1;
  PingablePtr pingable2;
  mojo::ConnectToService(shell(), GetURL("app?foo"), GetProxy(&pingable1));
  mojo::ConnectToService(shell(), GetURL("app?bar"), GetProxy(&pingable2));

  int num_responses = 0;
  auto callbacks_builder = [this, &num_responses](int query_index) {
    return [this, &num_responses, query_index](const String& app_url,
                                               const String& connection_url,
                                               const String& message) {
      EXPECT_EQ(GetURL("app"), app_url);
      EXPECT_EQ("hello", message);
      if (query_index == 1) {
        EXPECT_EQ(GetURL("app?foo"), connection_url);
      } else if (query_index == 2) {
        EXPECT_EQ(GetURL("app?bar"), connection_url);
      } else {
        CHECK(false);
      }
      ++num_responses;
      if (num_responses == 2)
        base::MessageLoop::current()->Quit();
    };
  };
  pingable1->Ping("hello", callbacks_builder(1));
  pingable2->Ping("hello", callbacks_builder(2));
  base::RunLoop().Run();
}

// mojo: URLs can have querystrings too
TEST_F(ShellAppTest, MojoURLQueryHandling) {
  PingablePtr pingable;
  mojo::ConnectToService(shell(), "mojo:pingable_app?foo", GetProxy(&pingable));
  auto callback = [](const String& app_url, const String& connection_url,
                     const String& message) {
    EXPECT_TRUE(base::EndsWith(app_url.To<base::StringPiece>(),
                               "/pingable_app.mojo",
                               base::CompareCase::SENSITIVE));
    EXPECT_EQ(app_url.To<std::string>() + "?foo", connection_url);
    EXPECT_EQ("hello", message);
    base::MessageLoop::current()->Quit();
  };
  pingable->Ping("hello", callback);
  base::RunLoop().Run();
}

void TestApplicationConnector(mojo::ApplicationConnector* app_connector) {
  PingablePtr pingable;
  ConnectToService(app_connector, "mojo:pingable_app", GetProxy(&pingable));
  auto callback = [](const String& app_url, const String& connection_url,
                     const String& message) {
    EXPECT_TRUE(base::EndsWith(app_url.To<base::StringPiece>(),
                               "/pingable_app.mojo",
                               base::CompareCase::SENSITIVE));
    EXPECT_EQ(app_url, connection_url);
    EXPECT_EQ("hello", message);
    base::MessageLoop::current()->Quit();
  };
  pingable->Ping("hello", callback);
  base::RunLoop().Run();
}

TEST_F(ShellAppTest, ApplicationConnector) {
  mojo::ApplicationConnectorPtr app_connector;
  app_connector.Bind(mojo::CreateApplicationConnector(shell()));
  TestApplicationConnector(app_connector.get());
}

TEST_F(ShellAppTest, ApplicationConnectorDuplicate) {
  mojo::ApplicationConnectorPtr app_connector1;
  app_connector1.Bind(mojo::CreateApplicationConnector(shell()));
  {
    SCOPED_TRACE("app_connector1");
    TestApplicationConnector(app_connector1.get());
  }

  mojo::ApplicationConnectorPtr app_connector2;
  app_connector1->Duplicate(GetProxy(&app_connector2));
  {
    SCOPED_TRACE("app_connector2");
    TestApplicationConnector(app_connector2.get());
  }

  // The first one should still work.
  {
    SCOPED_TRACE("app_connector1 again");
    TestApplicationConnector(app_connector1.get());
  }
}

}  // namespace
