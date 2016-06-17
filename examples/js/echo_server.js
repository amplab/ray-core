#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Demonstrate using the ServiceProvider returned by the Shell
// connectToApplication() method to provide a service to the
// target application and to request a service from the target
// application. To run this application with mojo_shell, set DIR
// to be the absolute path for this directory, then:
//   mojo_shell "file://$DIR/share_echo.js file://$DIR/share_echo_target.js"

define("main", [
  "console",
  "mojo/services/public/js/application",
  "examples/echo/echo.mojom",
], function(console, application, echo) {

  const Application = application.Application;
  const Echo = echo.Echo;
  var echoServerApp;

  class EchoImpl {
    echoString(s) {
      echoServerApp.quit();
      return Promise.resolve({value: s});
    }
  }

  class EchoServerApp extends Application {
    initialize(args) {
      echoServerApp = this;
    }

    acceptConnection(initiatorURL, initiatorServiceExchange) {
      initiatorServiceExchange.provideService(Echo, EchoImpl);
    }
  }

  return EchoServerApp;
});
