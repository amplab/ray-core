// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/http_server/cpp/http_server_util.h"
#include "mojo/services/http_server/interfaces/http_server.mojom-sync.h"
#include "mojo/services/http_server/interfaces/http_server_factory.mojom.h"
#include "mojo/services/network/interfaces/net_address.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "mojo/services/network/interfaces/url_loader.mojom.h"

namespace http_server {

namespace {

void WriteMessageToDataPipe(
    const std::string message,
    mojo::ScopedDataPipeConsumerHandle* data_pipe_consumer) {
  mojo::ScopedDataPipeProducerHandle producer_handle_;
  MojoCreateDataPipeOptions options = {sizeof(MojoCreateDataPipeOptions),
                                       MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,
                                       1,
                                       static_cast<uint32_t>(message.size())};

  MojoResult result =
      CreateDataPipe(&options, &producer_handle_, data_pipe_consumer);
  ASSERT_EQ(MOJO_RESULT_OK, result);

  uint32_t bytes = message.size();
  result = WriteDataRaw(producer_handle_.get(), message.data(), &bytes,
                        MOJO_WRITE_DATA_FLAG_ALL_OR_NONE);
  ASSERT_EQ(result, MOJO_RESULT_OK);
  ASSERT_EQ(message.size(), bytes);
}

const char kExampleMessage[] = "Hello, world!";

}  // namespace

// Test handler that responds to all requests with the status OK and
// kExampleMessage.
class GetHandler : public http_server::HttpHandler {
 public:
  GetHandler(mojo::InterfaceRequest<HttpHandler> request)
      : binding_(this, request.Pass()) {}
  ~GetHandler() override {}

 private:
  // http_server::HttpHandler:
  void HandleRequest(http_server::HttpRequestPtr request,
                     const mojo::Callback<void(http_server::HttpResponsePtr)>&
                         callback) override {
    callback.Run(http_server::CreateHttpResponse(200, kExampleMessage));
  }

  mojo::Binding<http_server::HttpHandler> binding_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(GetHandler);
};

// Test handler that responds to POST requests with the status OK and message
// read from the payload of the request.
class PostHandler : public http_server::HttpHandler {
 public:
  PostHandler(mojo::InterfaceRequest<HttpHandler> request)
      : binding_(this, request.Pass()) {}
  ~PostHandler() override {}

 private:
  // http_server::HttpHandler:
  void HandleRequest(http_server::HttpRequestPtr request,
                     const mojo::Callback<void(http_server::HttpResponsePtr)>&
                         callback) override {
    DCHECK_EQ("POST", request->method);
    std::string message;
    mojo::common::BlockingCopyToString(request->body.Pass(), &message);
    callback.Run(http_server::CreateHttpResponse(200, message));
  }

  mojo::Binding<http_server::HttpHandler> binding_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(PostHandler);
};

class HttpServerApplicationTest : public mojo::test::ApplicationTestBase {
 public:
  HttpServerApplicationTest() : ApplicationTestBase() {}
  ~HttpServerApplicationTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();

    mojo::ConnectToService(shell(), "mojo:http_server",
                           GetProxy(&http_server_factory_));
    mojo::ConnectToService(shell(), "mojo:network_service",
                           GetProxy(&network_service_));
  }

  mojo::SynchronousInterfacePtr<http_server::HttpServer> CreateHttpServer();

  http_server::HttpServerFactoryPtr http_server_factory_;
  mojo::NetworkServicePtr network_service_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(HttpServerApplicationTest);
};

mojo::SynchronousInterfacePtr<http_server::HttpServer>
HttpServerApplicationTest::CreateHttpServer() {
  mojo::SynchronousInterfacePtr<http_server::HttpServer> http_server;
  mojo::NetAddressPtr local_address(mojo::NetAddress::New());
  local_address->family = mojo::NetAddressFamily::IPV4;
  local_address->ipv4 = mojo::NetAddressIPv4::New();
  local_address->ipv4->addr.resize(4);
  local_address->ipv4->addr[0] = 127;
  local_address->ipv4->addr[1] = 0;
  local_address->ipv4->addr[2] = 0;
  local_address->ipv4->addr[3] = 1;
  local_address->ipv4->port = 0;
  http_server_factory_->CreateHttpServer(GetSynchronousProxy(&http_server),
                                         local_address.Pass());
  return http_server;
}

void CheckServerResponse(mojo::URLResponsePtr response) {
  EXPECT_EQ(200u, response->status_code);
  std::string response_body;
  mojo::common::BlockingCopyToString(response->body.Pass(), &response_body);
  EXPECT_EQ(kExampleMessage, response_body);
  base::MessageLoop::current()->Quit();
}

// Verifies that the server responds to http GET requests using example
// GetHandler.
TEST_F(HttpServerApplicationTest, ServerResponse) {
  auto http_server = CreateHttpServer();
  uint16_t assigned_port = 0;
  EXPECT_TRUE(http_server->GetPort(&assigned_port));
  EXPECT_NE(assigned_port, 0u);

  HttpHandlerPtr http_handler_ptr;
  GetHandler handler(GetProxy(&http_handler_ptr).Pass());

  // Set the test handler and wait for confirmation.
  bool result = false;
  EXPECT_TRUE(
      http_server->SetHandler("/test", http_handler_ptr.Pass(), &result));
  EXPECT_TRUE(result);

  mojo::URLLoaderPtr url_loader;
  network_service_->CreateURLLoader(GetProxy(&url_loader));

  mojo::URLRequestPtr url_request = mojo::URLRequest::New();
  url_request->url =
      base::StringPrintf("http://127.0.0.1:%u/test", assigned_port);
  url_loader->Start(url_request.Pass(), base::Bind(&CheckServerResponse));
  base::RunLoop run_loop;
  run_loop.Run();
}

// Verifies that the server correctly passes the POST request payload using
// example PostHandler.
TEST_F(HttpServerApplicationTest, PostData) {
  auto http_server = CreateHttpServer();
  uint16_t assigned_port = 0;
  EXPECT_TRUE(http_server->GetPort(&assigned_port));
  EXPECT_NE(assigned_port, 0u);

  HttpHandlerPtr http_handler_ptr;
  PostHandler handler(GetProxy(&http_handler_ptr).Pass());

  // Set the test handler and wait for confirmation.
  bool result = false;
  EXPECT_TRUE(
      http_server->SetHandler("/post", http_handler_ptr.Pass(), &result));
  EXPECT_TRUE(result);

  mojo::URLLoaderPtr url_loader;
  network_service_->CreateURLLoader(GetProxy(&url_loader));

  mojo::URLRequestPtr url_request = mojo::URLRequest::New();
  url_request->url =
      base::StringPrintf("http://127.0.0.1:%u/post", assigned_port);
  url_request->method = "POST";
  url_request->body.resize(1);
  WriteMessageToDataPipe(kExampleMessage, &url_request->body[0]);

  url_loader->Start(url_request.Pass(), base::Bind(&CheckServerResponse));
  base::RunLoop run_loop;
  run_loop.Run();
}

}  // namespace http_server
