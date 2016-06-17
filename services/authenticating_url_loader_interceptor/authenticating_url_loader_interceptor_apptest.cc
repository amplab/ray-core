// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/atomic_sequence_num.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/services/authenticating_url_loader_interceptor/interfaces/authenticating_url_loader_interceptor_meta_factory.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {

namespace {

base::StaticAtomicSequenceNumber g_url_index_;

const char kMessage[] = "Hello World\n";
const char kUser[] = "johnsmith@gmail.com";
const char kCachedToken[] = "this_is_a_cached_token";
const char kFreshToken[] = "this_is_a_fresh_token";
const char kAuthenticationScope[] =
    "https://www.googleapis.com/auth/userinfo.email";
const char kAuthenticationHeaderName[] = "Authorization";
const char kAuthenticationHeaderValuePrefix[] = "Bearer";

class MockAuthenticationService : public authentication::AuthenticationService {
 public:
  MockAuthenticationService(
      mojo::InterfaceRequest<authentication::AuthenticationService> request)
      : binding_(this, request.Pass()),
        num_select_account_calls_(0),
        num_get_token_calls_(0),
        num_clear_token_calls_(0),
        use_fresh_token_(false),
        close_pipe_on_user_selection_(false),
        return_error_on_user_selection_(false),
        return_error_on_token_retrieval_(false) {
    // The AuthenticationService should never be closed from the other side in
    // these tests.
    binding_.set_connection_error_handler([this]() { DCHECK(0); });
  }
  ~MockAuthenticationService() override {}

  void set_close_pipe_on_user_selection() {
    close_pipe_on_user_selection_ = true;
  }
  void set_return_error_on_user_selection() {
    return_error_on_user_selection_ = true;
  }
  void set_return_error_on_token_retrieval() {
    return_error_on_token_retrieval_ = true;
  }

  uint32_t num_select_account_calls() { return num_select_account_calls_; }
  uint32_t num_get_token_calls() { return num_get_token_calls_; }
  uint32_t num_clear_token_calls() { return num_clear_token_calls_; }

 private:
  // AuthenticationService implementation
  void SelectAccount(bool return_last_selected,
                     const SelectAccountCallback& callback) override {
    num_select_account_calls_++;
    if (close_pipe_on_user_selection_) {
      binding_.Close();
      return;
    }

    if (return_error_on_user_selection_) {
      callback.Run(nullptr, "error selecting user");
      return;
    }

    callback.Run(kUser, nullptr);
  }
  void GetOAuth2Token(const mojo::String& username,
                      mojo::Array<mojo::String> scopes,
                      const GetOAuth2TokenCallback& callback) override {
    num_get_token_calls_++;
    EXPECT_EQ(kUser, username);
    EXPECT_EQ(1u, scopes.size());
    if (scopes.size())
      EXPECT_EQ(kAuthenticationScope, scopes[0]);

    if (return_error_on_token_retrieval_) {
      callback.Run(nullptr, "error selecting token");
      return;
    }

    if (use_fresh_token_) {
      callback.Run(kFreshToken, nullptr);
      return;
    }
    callback.Run(kCachedToken, nullptr);
  }
  void ClearOAuth2Token(const mojo::String& token) override {
    num_clear_token_calls_++;
    use_fresh_token_ = true;
  }

  Binding<authentication::AuthenticationService> binding_;
  uint32_t num_select_account_calls_;
  uint32_t num_get_token_calls_;
  uint32_t num_clear_token_calls_;
  bool use_fresh_token_;
  bool close_pipe_on_user_selection_;
  bool return_error_on_user_selection_;
  bool return_error_on_token_retrieval_;
};

class BaseInterceptor : public URLLoaderInterceptor {
 public:
  BaseInterceptor(InterfaceRequest<URLLoaderInterceptor> request)
      : binding_(this, request.Pass()) {}
  ~BaseInterceptor() override {}

 protected:
  void InterceptRequest(URLRequestPtr request,
                        const InterceptRequestCallback& callback) override {
    URLLoaderInterceptorResponsePtr interceptor_response =
        URLLoaderInterceptorResponse::New();
    interceptor_response->request = request.Pass();
    callback.Run(interceptor_response.Pass());
  }

  void InterceptResponse(URLResponsePtr response,
                         const InterceptResponseCallback& callback) override {
    URLLoaderInterceptorResponsePtr interceptor_response =
        URLLoaderInterceptorResponse::New();
    interceptor_response->response = response.Pass();
    callback.Run(interceptor_response.Pass());
  }

  void InterceptFollowRedirect(
      const InterceptFollowRedirectCallback& callback) override {
    callback.Run(URLLoaderInterceptorResponsePtr());
  }

 private:
  StrongBinding<URLLoaderInterceptor> binding_;
};

class SendHelloInterceptor : public BaseInterceptor {
 public:
  SendHelloInterceptor(InterfaceRequest<URLLoaderInterceptor> request)
      : BaseInterceptor(request.Pass()) {}
  ~SendHelloInterceptor() override {}

 private:
  void InterceptRequest(URLRequestPtr request,
                        const InterceptRequestCallback& callback) override {
    URLResponsePtr response = URLResponse::New();
    response->url = request->url;
    response->status_code = 200;
    response->status_line = "200 OK";
    response->mime_type = "text/plain";
    response->headers = request->headers.Pass();
    uint32_t num_bytes = arraysize(kMessage);
    MojoCreateDataPipeOptions options;
    options.struct_size = sizeof(MojoCreateDataPipeOptions);
    options.flags = MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE;
    options.element_num_bytes = 1;
    options.capacity_num_bytes = num_bytes;
    DataPipe data_pipe(options);
    response->body = data_pipe.consumer_handle.Pass();
    MojoResult result =
        WriteDataRaw(data_pipe.producer_handle.get(), kMessage, &num_bytes,
                     MOJO_WRITE_DATA_FLAG_ALL_OR_NONE);
    EXPECT_EQ(MOJO_RESULT_OK, result);

    URLLoaderInterceptorResponsePtr interceptor_response =
        URLLoaderInterceptorResponse::New();
    interceptor_response->response = response.Pass();
    callback.Run(interceptor_response.Pass());
  }
};

class PassThroughIfHasTokenInterceptor : public BaseInterceptor {
 public:
  PassThroughIfHasTokenInterceptor(
      InterfaceRequest<URLLoaderInterceptor> request,
      const std::string& desired_token)
      : BaseInterceptor(request.Pass()), desired_token_(desired_token) {}
  ~PassThroughIfHasTokenInterceptor() override {}

 private:
  void InterceptRequest(URLRequestPtr request,
                        const InterceptRequestCallback& callback) override {
    URLLoaderInterceptorResponsePtr interceptor_response =
        URLLoaderInterceptorResponse::New();

    // Check that authentication is present, and if so, let the request
    // through.
    bool found_authentication = false;
    if (request->headers.size()) {
      EXPECT_EQ(1u, request->headers.size());
      HttpHeaderPtr header = request->headers[0].Pass();
      EXPECT_EQ(kAuthenticationHeaderName, header->name);

      std::vector<std::string> auth_value_components =
          base::SplitString(header->value.To<std::string>(), " ",
                            base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
      EXPECT_EQ(2u, auth_value_components.size());
      if (auth_value_components.size() == 2) {
        EXPECT_EQ(kAuthenticationHeaderValuePrefix, auth_value_components[0]);
        if (auth_value_components[1] == desired_token_) {
          found_authentication = true;
        }
      }
    }

    if (found_authentication) {
      request->headers.reset();
      interceptor_response->request = request.Pass();
    } else {
      // Send a response indicating that authentication is required.
      URLResponsePtr response = URLResponse::New();
      response->url = request->url;
      response->status_code = 401;
      response->status_line = "401 Authorization Required";
      interceptor_response->response = response.Pass();
    }

    callback.Run(interceptor_response.Pass());
  }

  std::string desired_token_;
};

class PassThroughIfHasCachedTokenInterceptor
    : public PassThroughIfHasTokenInterceptor {
 public:
  PassThroughIfHasCachedTokenInterceptor(
      InterfaceRequest<URLLoaderInterceptor> request)
      : PassThroughIfHasTokenInterceptor(request.Pass(), kCachedToken) {}
  ~PassThroughIfHasCachedTokenInterceptor() override {}
};

class PassThroughIfHasFreshTokenInterceptor
    : public PassThroughIfHasTokenInterceptor {
 public:
  PassThroughIfHasFreshTokenInterceptor(
      InterfaceRequest<URLLoaderInterceptor> request)
      : PassThroughIfHasTokenInterceptor(request.Pass(), kFreshToken) {}
  ~PassThroughIfHasFreshTokenInterceptor() override {}
};

template <class I>
class URLLoaderInterceptorFactoryImpl : public URLLoaderInterceptorFactory {
 public:
  URLLoaderInterceptorFactoryImpl(
      InterfaceRequest<URLLoaderInterceptorFactory> request)
      : binding_(this, request.Pass()) {}
  ~URLLoaderInterceptorFactoryImpl() override {}

 private:
  void Create(
      mojo::InterfaceRequest<URLLoaderInterceptor> interceptor) override {
    new I(interceptor.Pass());
  }

  StrongBinding<URLLoaderInterceptorFactory> binding_;
};

class AuthenticatingURLLoaderInterceptorAppTest
    : public test::ApplicationTestBase {
 public:
  AuthenticatingURLLoaderInterceptorAppTest() {}
  ~AuthenticatingURLLoaderInterceptorAppTest() override {}

  void SetUp() override {
    ApplicationTestBase::SetUp();

    InitializeNetworkService();
    ConnectToService(shell(), "mojo:authenticating_url_loader_interceptor",
                     GetProxy(&interceptor_meta_factory_));
  }

  void TearDown() override {
    // Close the AuthenticationService explicitly here so that teardown code
    // doesn't cause it to receive a connection error.
    CloseAuthenticationService();
    ApplicationTestBase::TearDown();
  }

  URLResponsePtr GetResponse(const std::string& url,
                             Array<HttpHeaderPtr> request_headers = nullptr) {
    URLRequestPtr request = URLRequest::New();
    request->url = url;
    request->headers = request_headers.Pass();
    URLLoaderPtr loader;
    network_service_->CreateURLLoader(GetProxy(&loader));
    URLResponsePtr response;
    {
      base::RunLoop loop;
      loader->Start(request.Pass(), [&response, &loop](URLResponsePtr r) {
        response = r.Pass();
        loop.Quit();

      });
      loop.Run();
    }
    return response;
  }

  template <class I>
  void AddInterceptor() {
    URLLoaderInterceptorFactoryPtr factory;
    new URLLoaderInterceptorFactoryImpl<I>(GetProxy(&factory));
    network_service_->RegisterURLLoaderInterceptor(factory.Pass());
  }

  void AddAuthenticatingURLLoaderInterceptor() {
    authentication::AuthenticationServicePtr authentication_service;
    authentication_service_impl_.reset(
        new MockAuthenticationService(GetProxy(&authentication_service)));
    mojo::URLLoaderInterceptorFactoryPtr interceptor_factory;
    interceptor_meta_factory_->CreateURLLoaderInterceptorFactory(
        GetProxy(&interceptor_factory), authentication_service.Pass());
    network_service_->RegisterURLLoaderInterceptor(interceptor_factory.Pass());
  }

  void InitializeNetworkService() {
    network_service_.reset();
    mojo::ConnectToService(shell(), "mojo:network_service",
                           GetProxy(&network_service_));
  }

  void CloseAuthenticationService() { authentication_service_impl_.reset(); }

 protected:
  std::string GetNextURL() {
    return base::StringPrintf("http://www.example%d.com/path/to/url",
                              g_url_index_.GetNext());
  }

  void GetAndParseHelloResponse(
      const std::string& url,
      Array<HttpHeaderPtr> request_headers = nullptr) {
    URLResponsePtr response = GetResponse(url, request_headers.Pass());
    EXPECT_TRUE(response);
    EXPECT_EQ(200u, response->status_code);
    EXPECT_EQ(url, response->url);
    EXPECT_EQ(0u, response->headers.size());
    char received_message[arraysize(kMessage)];
    uint32_t num_bytes = arraysize(kMessage);
    EXPECT_EQ(MOJO_RESULT_OK,
              ReadDataRaw(response->body.get(), received_message, &num_bytes,
                          MOJO_READ_DATA_FLAG_NONE));
    EXPECT_EQ(arraysize(kMessage), num_bytes);
    EXPECT_EQ(0, memcmp(kMessage, received_message, num_bytes));
  }

  NetworkServicePtr network_service_;
  AuthenticatingURLLoaderInterceptorMetaFactoryPtr interceptor_meta_factory_;
  std::unique_ptr<MockAuthenticationService> authentication_service_impl_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatingURLLoaderInterceptorAppTest);
};

}  // namespace

// Test that the authenticating interceptor passes through a response that
// does not indicate that authentication is required if authentication is not
// available.
TEST_F(AuthenticatingURLLoaderInterceptorAppTest, AuthenticationNotRequired) {
  AddInterceptor<SendHelloInterceptor>();
  AddAuthenticatingURLLoaderInterceptor();
  CloseAuthenticationService();

  GetAndParseHelloResponse(GetNextURL());
}

// Test that the authenticating interceptor passes through a response that
// indicates that authentication is required if authentication is not
// available.
TEST_F(AuthenticatingURLLoaderInterceptorAppTest, AuthenticationNotAvailable) {
  AddInterceptor<SendHelloInterceptor>();
  AddInterceptor<PassThroughIfHasCachedTokenInterceptor>();
  AddAuthenticatingURLLoaderInterceptor();
  CloseAuthenticationService();

  std::string url = GetNextURL();
  URLResponsePtr response = GetResponse(url);
  EXPECT_TRUE(response);
  EXPECT_EQ(401u, response->status_code);
  EXPECT_EQ(url, response->url);
  EXPECT_EQ(0u, response->headers.size());
}

// Test that the authenticating interceptor adds an authentication header to  a
// response that indicates that authentication is required if authentication is
// available.
TEST_F(AuthenticatingURLLoaderInterceptorAppTest, AuthenticationAvailable) {
  AddInterceptor<SendHelloInterceptor>();
  AddInterceptor<PassThroughIfHasCachedTokenInterceptor>();
  AddAuthenticatingURLLoaderInterceptor();

  GetAndParseHelloResponse(GetNextURL());
  EXPECT_EQ(1u, authentication_service_impl_->num_select_account_calls());
  EXPECT_EQ(1u, authentication_service_impl_->num_get_token_calls());
  EXPECT_EQ(0u, authentication_service_impl_->num_clear_token_calls());
}

// Test that the authenticating interceptor fails a request needing
// authentication if user selection returns an error.
TEST_F(AuthenticatingURLLoaderInterceptorAppTest, FailSelectingUser) {
  AddInterceptor<SendHelloInterceptor>();
  AddInterceptor<PassThroughIfHasCachedTokenInterceptor>();
  AddAuthenticatingURLLoaderInterceptor();
  authentication_service_impl_->set_return_error_on_user_selection();

  std::string url = GetNextURL();
  URLResponsePtr response = GetResponse(url);
  EXPECT_TRUE(response);
  EXPECT_EQ(401u, response->status_code);
  EXPECT_EQ(url, response->url);
  EXPECT_EQ(0u, response->headers.size());
  EXPECT_EQ(1u, authentication_service_impl_->num_select_account_calls());
  EXPECT_EQ(0u, authentication_service_impl_->num_get_token_calls());
  EXPECT_EQ(0u, authentication_service_impl_->num_clear_token_calls());
}

// Test that the authenticating interceptor fails a request needing
// authentication if token retrieval returns an error.
TEST_F(AuthenticatingURLLoaderInterceptorAppTest, FailGettingToken) {
  AddInterceptor<SendHelloInterceptor>();
  AddInterceptor<PassThroughIfHasCachedTokenInterceptor>();
  AddAuthenticatingURLLoaderInterceptor();
  authentication_service_impl_->set_return_error_on_token_retrieval();

  std::string url = GetNextURL();
  URLResponsePtr response = GetResponse(url);
  EXPECT_TRUE(response);
  EXPECT_EQ(401u, response->status_code);
  EXPECT_EQ(url, response->url);
  EXPECT_EQ(0u, response->headers.size());
  EXPECT_EQ(1u, authentication_service_impl_->num_select_account_calls());
  EXPECT_EQ(1u, authentication_service_impl_->num_get_token_calls());
  EXPECT_EQ(0u, authentication_service_impl_->num_clear_token_calls());
}

// Test that the authenticating interceptor caches tokens in memory so that a
// second request for an authenticated resource does not result in the
// AuthenticationService being contacted.
TEST_F(AuthenticatingURLLoaderInterceptorAppTest, CacheToken) {
  AddInterceptor<SendHelloInterceptor>();
  AddInterceptor<PassThroughIfHasCachedTokenInterceptor>();
  AddAuthenticatingURLLoaderInterceptor();

  std::string url = GetNextURL();
  GetAndParseHelloResponse(url);
  EXPECT_EQ(1u, authentication_service_impl_->num_select_account_calls());
  EXPECT_EQ(1u, authentication_service_impl_->num_get_token_calls());
  EXPECT_EQ(0u, authentication_service_impl_->num_clear_token_calls());

  GetAndParseHelloResponse(url);
  EXPECT_EQ(1u, authentication_service_impl_->num_select_account_calls());
  EXPECT_EQ(1u, authentication_service_impl_->num_get_token_calls());
  EXPECT_EQ(0u, authentication_service_impl_->num_clear_token_calls());
}

// Test that the authenticating interceptor obtains a fresh token if a request
// to an authenticated resource fails with a cached token.
TEST_F(AuthenticatingURLLoaderInterceptorAppTest, ClearCachedToken) {
  AddInterceptor<SendHelloInterceptor>();
  AddInterceptor<PassThroughIfHasFreshTokenInterceptor>();
  AddAuthenticatingURLLoaderInterceptor();

  GetAndParseHelloResponse(GetNextURL());
  EXPECT_EQ(1u, authentication_service_impl_->num_select_account_calls());
  EXPECT_EQ(2u, authentication_service_impl_->num_get_token_calls());
  EXPECT_EQ(1u, authentication_service_impl_->num_clear_token_calls());
}

// Test that the authenticating interceptor fails any outstanding requests if
// the connection to the authentication service is lost.
TEST_F(AuthenticatingURLLoaderInterceptorAppTest, FailOnAuthServiceClosing) {
  AddInterceptor<SendHelloInterceptor>();
  AddInterceptor<PassThroughIfHasFreshTokenInterceptor>();
  AddAuthenticatingURLLoaderInterceptor();
  authentication_service_impl_->set_close_pipe_on_user_selection();

  std::string url = GetNextURL();
  URLResponsePtr response = GetResponse(url);
  EXPECT_TRUE(response);
  EXPECT_EQ(401u, response->status_code);
  EXPECT_EQ(url, response->url);
  EXPECT_EQ(0u, response->headers.size());
  EXPECT_EQ(1u, authentication_service_impl_->num_select_account_calls());
  EXPECT_EQ(0u, authentication_service_impl_->num_get_token_calls());
  EXPECT_EQ(0u, authentication_service_impl_->num_clear_token_calls());
}

// Test that the authenticating interceptor does not add an auth header if the
// request already has one or modify the one that the request has.
TEST_F(AuthenticatingURLLoaderInterceptorAppTest, RequestWithAuthHeader) {
  std::string url = GetNextURL();

  // Set up the interceptor to cache |kCachedToken| for |url|.
  AddInterceptor<SendHelloInterceptor>();
  AddInterceptor<PassThroughIfHasCachedTokenInterceptor>();
  AddAuthenticatingURLLoaderInterceptor();

  GetAndParseHelloResponse(url);
  EXPECT_EQ(1u, authentication_service_impl_->num_select_account_calls());
  EXPECT_EQ(1u, authentication_service_impl_->num_get_token_calls());
  EXPECT_EQ(0u, authentication_service_impl_->num_clear_token_calls());

  // Reset the network service, set things up so that a different token is
  // required for authentication, and send off a request that has that token.
  InitializeNetworkService();
  AddInterceptor<SendHelloInterceptor>();
  AddInterceptor<PassThroughIfHasFreshTokenInterceptor>();
  AddAuthenticatingURLLoaderInterceptor();

  auto auth_header = HttpHeader::New();
  auth_header->name = "Authorization";
  auth_header->value = "Bearer " + std::string(kFreshToken);
  Array<HttpHeaderPtr> request_headers;
  request_headers.push_back(auth_header.Pass());
  GetAndParseHelloResponse(url, request_headers.Pass());
  EXPECT_EQ(0u, authentication_service_impl_->num_select_account_calls());
  EXPECT_EQ(0u, authentication_service_impl_->num_get_token_calls());
  EXPECT_EQ(0u, authentication_service_impl_->num_clear_token_calls());
}

}  // namespace mojo
