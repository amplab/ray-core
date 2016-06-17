#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Demonstrate using the ServiceProvider returned by the Shell
// connectToApplication() method to provide a service to the
// target application and to request a service from the target
// application. To run this application with mojo_shell:
//   mojo_shell file://full/path/to/share_echo.js

define("main", [
  "console",
  "mojo/services/public/js/application",
  "examples/echo/echo.mojom",
], function(console, application, echo) {

  const Application = application.Application;
  const Echo = echo.Echo;

  var shareEchoApp;

  class ShareEcho extends Application {
    initialize(args) {
      // The Application object is a singleton. Remember it.
      shareEchoApp = this;

      var shareEchoTargetURL =
          this.url.replace("share_echo.js", "share_echo_target.js");

      // The value of targetSP is-a JS ServiceProvider that's connected to the
      // share_echo_target.js application. We provide our implementation of
      // Echo to the share_echo_target.js application and request its
      // Echo implementation.
      var targetSP = this.shell.connectToApplication(shareEchoTargetURL);

      // Get the implementation at the other end, call it, and prepare for
      // the async callback.
      var echoService = targetSP.requestService(Echo);
      echoService.echoString("ShareEcho").then(function(response) {
        console.log(response.value);
        shareEchoApp.quit();
      });
    }
  }

  return ShareEcho;
});
