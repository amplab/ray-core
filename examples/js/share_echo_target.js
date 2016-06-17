#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Demonstrate sharing services via application ServiceProviders.
// This application is launched by share_echo.js. It both provides and
// requests an Echo implementation. See share_echo.js for instructions.

define("main", [
  "console",
  "mojo/services/public/js/application",
  "examples/echo/echo.mojom",
], function(console, application, echo) {

  const Application = application.Application;
  const Echo = echo.Echo;

  var shareEchoTargetApp;

  class EchoImpl {
    echoString(s) {
      shareEchoTargetApp.quit();
      return Promise.resolve({value: "ShareEchoTarget: " + s});
    }
  }

  class ShareEchoTarget extends Application {
    initialize(args) {
      shareEchoTargetApp = this;
    }

    acceptConnection(initiatorURL, shareEchoServiceExchange) {
      shareEchoServiceExchange.provideService(Echo, EchoImpl);
    }
  }

  return ShareEchoTarget;
});
