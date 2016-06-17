// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_handle.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/services/http_server/cpp/http_server_util.h"
#include "mojo/services/http_server/interfaces/http_server.mojom.h"
#include "mojo/services/http_server/interfaces/http_server_factory.mojom.h"
#include "mojo/services/network/interfaces/net_address.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "mojo/services/network/interfaces/url_loader.mojom.h"

namespace mojo_url_redirector {

namespace {
const std::string kPlatform1 = "platform1";
const std::string kPlatform2 = "platform2";
const std::string kKnownAppName = "spinning_cube.mojo";
const std::string kMissingAppName = "missing_app.mojo";
const std::string kGoogleStorageBaseURL = "https://storage.googleapis.com/";

std::string LocationOfAppOnPlatform(const std::string& platform,
                                    const std::string& app) {
  return platform + "-" + app;
}

void CheckRedirectorResponse(uint32 expected_http_status,
                             std::string expected_redirect,
                             mojo::URLResponsePtr response) {
  // Break out of the nested runloop that the test started running after
  // starting the request that is now complete.
  base::MessageLoop::current()->Quit();

  EXPECT_FALSE(response->error);
  EXPECT_EQ(expected_http_status, response->status_code);
  std::string response_body;
  mojo::common::BlockingCopyToString(response->body.Pass(), &response_body);
  EXPECT_EQ("", response_body);

  if (expected_http_status != 302)
    return;

  // Check that the response contains a header redirecting to the expected
  // location.
  bool found_redirect_header = false;
  EXPECT_FALSE(response->headers.is_null());
  for (size_t i = 0; i < response->headers.size(); i++) {
    const auto& header = response->headers[i];
    if (header->name == "location" && header->value == expected_redirect) {
      found_redirect_header = true;
      break;
    }
  }
  EXPECT_TRUE(found_redirect_header);
}

}  // namespace

class MojoUrlRedirectorApplicationTest :
    public mojo::test::ApplicationTestBase,
    public http_server::HttpHandler {
 public:
  MojoUrlRedirectorApplicationTest() : ApplicationTestBase(),
      binding_(this),
      redirector_port_(0),
      redirector_registered_(false) {}
  ~MojoUrlRedirectorApplicationTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();

    // Obtain the port that the redirector is at and the port that we should
    // spin up the app location files server at.
    uint16_t app_location_files_port = 0;
    for (const std::string& arg : args()) {
      if (arg.find("--redirector_port") != std::string::npos) {
        sscanf(arg.c_str(), "--redirector_port=%hu", &redirector_port_);
      } else if (arg.find("--app_location_files_port") != std::string::npos) {
        sscanf(arg.c_str(), "--app_location_files_port=%hu",
               &app_location_files_port);
      }
    }
    DCHECK(redirector_port_);
    DCHECK(app_location_files_port);

    // Spin up the app location files server.
    http_server::HttpHandlerPtr location_files_handler;
    binding_.Bind(GetProxy(&location_files_handler));

    http_server::HttpServerFactoryPtr http_server_factory;
    mojo::ConnectToService(shell(), "mojo:http_server",
                           GetProxy(&http_server_factory));

    mojo::NetAddressPtr location_files_server_addr(mojo::NetAddress::New());
    location_files_server_addr->family = mojo::NetAddressFamily::IPV4;
    location_files_server_addr->ipv4 = mojo::NetAddressIPv4::New();
    location_files_server_addr->ipv4->addr.resize(4);
    location_files_server_addr->ipv4->addr[0] = 0;
    location_files_server_addr->ipv4->addr[1] = 0;
    location_files_server_addr->ipv4->addr[2] = 0;
    location_files_server_addr->ipv4->addr[3] = 0;
    location_files_server_addr->ipv4->port = app_location_files_port;
    http_server_factory->CreateHttpServer(
        GetProxy(&location_files_server_).Pass(),
        location_files_server_addr.Pass());

    location_files_server_->SetHandler(
        "/.*",
        location_files_handler.Pass(),
        base::Bind(&MojoUrlRedirectorApplicationTest::OnAddedHandler,
                   base::Unretained(this)));
    location_files_server_.WaitForIncomingResponse();

    // Connect to the redirector and wait until it registers itself as a
    // handler with the server on |redirector_port_|.
    shell()->ConnectToApplication("mojo:mojo_url_redirector",
                                  GetProxy(&url_redirector_sp_));
    mojo::ConnectToService(shell(), "mojo:network_service",
                           GetProxy(&network_service_));
    WaitForRedirectorRegistration();
  }

  void TestRedirectForKnownApp(const std::string& platform,
                               const std::string& app);

  mojo::Binding<http_server::HttpHandler> binding_;
  mojo::InterfaceHandle<mojo::ServiceProvider> url_redirector_sp_;
  mojo::NetworkServicePtr network_service_;
  uint16_t redirector_port_;
  bool redirector_registered_;

 private:
  // HttpHandler:
  void HandleRequest(
      http_server::HttpRequestPtr request,
      const mojo::Callback<void(http_server::HttpResponsePtr)>& callback)
          override;

  void WaitForRedirectorRegistration();
  void CheckRedirectorRegistered(mojo::URLResponsePtr response);
  void OnAddedHandler(bool result);

  http_server::HttpServerPtr location_files_server_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(MojoUrlRedirectorApplicationTest);
};

void MojoUrlRedirectorApplicationTest::OnAddedHandler(bool result) {
  CHECK(result);
}

  // Handles requests for app location files.
void MojoUrlRedirectorApplicationTest::HandleRequest(
      http_server::HttpRequestPtr request,
      const mojo::Callback<void(http_server::HttpResponsePtr)>& callback) {
  // The relative url should be of the form "/<platform>/<app>_location".
  std::vector<std::string> url_components =
      base::SplitString(request->relative_url.get(), "/", base::TRIM_WHITESPACE,
                        base::SPLIT_WANT_ALL);
  ASSERT_EQ(3u, url_components.size());
  std::string requested_platform = url_components[1];
  EXPECT_TRUE(requested_platform == kPlatform1 ||
              requested_platform == kPlatform2);

  std::string location_file_basename = url_components[2];
  std::string known_app_location_file = kKnownAppName + "_location";
  std::string missing_app_location_file = kMissingAppName + "_location";
  EXPECT_TRUE(location_file_basename == known_app_location_file ||
              location_file_basename == missing_app_location_file);

  if (location_file_basename == missing_app_location_file) {
    callback.Run(http_server::CreateHttpResponse(404, ""));
    return;
  }

  std::string app_location = LocationOfAppOnPlatform(requested_platform,
                                                     kKnownAppName);
  callback.Run(http_server::CreateHttpResponse(200, app_location));
}

void MojoUrlRedirectorApplicationTest::WaitForRedirectorRegistration() {
  while (!redirector_registered_) {
    mojo::URLLoaderPtr url_loader;
    network_service_->CreateURLLoader(GetProxy(&url_loader));
    mojo::URLRequestPtr url_request = mojo::URLRequest::New();
    url_request->url =
        base::StringPrintf("http://localhost:%u/test", redirector_port_);
    url_loader->Start(url_request.Pass(),
        base::Bind(
            &MojoUrlRedirectorApplicationTest::CheckRedirectorRegistered,
            base::Unretained(this)));
    ASSERT_TRUE(url_loader.WaitForIncomingResponse());
  }
}

void MojoUrlRedirectorApplicationTest::CheckRedirectorRegistered(
    mojo::URLResponsePtr response) {
  if (response->error) {
    // The server at |redirector_port_| has not yet been spun up.
    return;
  }

  if (response->status_code == 404) {
    // The redirector has not yet been registered as a handler.
    return;
  }

  redirector_registered_ = true;
}

void MojoUrlRedirectorApplicationTest::TestRedirectForKnownApp(
    const std::string& platform, const std::string& app) {
  mojo::URLLoaderPtr url_loader;
  network_service_->CreateURLLoader(GetProxy(&url_loader));

  mojo::URLRequestPtr url_request = mojo::URLRequest::New();
  url_request->url =
      base::StringPrintf("http://localhost:%u/%s/%s",
                         redirector_port_,
                         platform.c_str(),
                         app.c_str());

  std::string app_location = kGoogleStorageBaseURL +
      LocationOfAppOnPlatform(platform, app);
  url_loader->Start(url_request.Pass(), base::Bind(&CheckRedirectorResponse,
                                                   302, app_location));
  base::RunLoop run_loop;
  run_loop.Run();
}

// TODO(blundell): This test is flaky.
// https://github.com/domokit/mojo/issues/115
TEST_F(MojoUrlRedirectorApplicationTest, DISABLED_MalformedRequest) {
  mojo::URLLoaderPtr url_loader;
  network_service_->CreateURLLoader(GetProxy(&url_loader));

  mojo::URLRequestPtr url_request = mojo::URLRequest::New();
  url_request->url =
      base::StringPrintf("http://localhost:%u/test", redirector_port_);
  url_loader->Start(url_request.Pass(), base::Bind(&CheckRedirectorResponse,
                                                   400, ""));
  base::RunLoop run_loop;
  run_loop.Run();
}

// TODO(blundell): This test is flaky.
// https://github.com/domokit/mojo/issues/115
TEST_F(MojoUrlRedirectorApplicationTest, DISABLED_RequestForMissingApp) {
  mojo::URLLoaderPtr url_loader;
  network_service_->CreateURLLoader(GetProxy(&url_loader));

  mojo::URLRequestPtr url_request = mojo::URLRequest::New();
  url_request->url =
      base::StringPrintf("http://localhost:%u/%s/%s",
                         redirector_port_,
                         kPlatform1.c_str(),
                         kMissingAppName.c_str());
  url_loader->Start(url_request.Pass(), base::Bind(&CheckRedirectorResponse,
                                                   404, ""));
  base::RunLoop run_loop;
  run_loop.Run();
}

// TODO(blundell): This test is flaky.
// https://github.com/domokit/mojo/issues/115
TEST_F(MojoUrlRedirectorApplicationTest, DISABLED_RequestForKnownApp) {
  TestRedirectForKnownApp(kPlatform1, kKnownAppName);
  TestRedirectForKnownApp(kPlatform2, kKnownAppName);
}

}  // namespace mojo_url_redirector
