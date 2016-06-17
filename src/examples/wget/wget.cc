// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/system/wait.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "mojo/services/network/interfaces/url_loader.mojom.h"

namespace mojo {
namespace examples {
namespace {

class ResponsePrinter {
 public:
  void Run(URLResponsePtr response) const {
    if (response->error) {
      printf("Got error: %d (%s)\n",
          response->error->code, response->error->description.get().c_str());
    } else {
      PrintResponse(response);
      PrintResponseBody(response->body.Pass());
    }

    RunLoop::current()->Quit();  // All done!
  }

  void PrintResponse(const URLResponsePtr& response) const {
    printf(">>> Headers <<< \n");
    printf("  %s\n", response->status_line.get().c_str());
    if (response->headers) {
      for (size_t i = 0; i < response->headers.size(); ++i)
        printf("  %s=%s\n",
               response->headers[i]->name.To<std::string>().c_str(),
               response->headers[i]->value.To<std::string>().c_str());
    }
  }

  void PrintResponseBody(ScopedDataPipeConsumerHandle body) const {
    // Read response body in blocking fashion.
    printf(">>> Body <<<\n");

    for (;;) {
      char buf[512];
      uint32_t num_bytes = sizeof(buf);
      MojoResult result = ReadDataRaw(body.get(), buf, &num_bytes,
                                      MOJO_READ_DATA_FLAG_NONE);
      if (result == MOJO_RESULT_SHOULD_WAIT) {
        Wait(body.get(),
             MOJO_HANDLE_SIGNAL_READABLE,
             MOJO_DEADLINE_INDEFINITE,
             nullptr);
      } else if (result == MOJO_RESULT_OK) {
        if (fwrite(buf, num_bytes, 1, stdout) != 1) {
          printf("\nUnexpected error writing to file\n");
          break;
        }
      } else {
        break;
      }
    }

    printf("\n>>> EOF <<<\n");
  }
};

}  // namespace

class WGetApp : public ApplicationImplBase {
 public:
  void OnInitialize() override {
    ConnectToService(shell(), "mojo:network_service",
                     GetProxy(&network_service_));
    Start(args());
  }

 private:
  void Start(const std::vector<std::string>& args) {
    std::string url((args.size() > 1) ? args[1] : PromptForURL());
    printf("Loading: %s\n", url.c_str());

    network_service_->CreateURLLoader(GetProxy(&url_loader_));

    URLRequestPtr request(URLRequest::New());
    request->url = url;
    request->method = "GET";
    request->auto_follow_redirects = true;

    url_loader_->Start(request.Pass(), ResponsePrinter());
  }

  std::string PromptForURL() {
    printf("Enter URL> ");
    char buf[1024];
    if (scanf("%1023s", buf) != 1)
      buf[0] = '\0';
    return buf;
  }

  NetworkServicePtr network_service_;
  URLLoaderPtr url_loader_;
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::examples::WGetApp wget_app;
  return mojo::RunApplication(application_request, &wget_app);
}
