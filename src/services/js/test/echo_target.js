#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

define("main", [
  "mojo/services/public/js/application",
  "services/js/test/echo_service.mojom",
], function(application, echoServiceMojom) {

  const Application = application.Application;
  const EchoService = echoServiceMojom.EchoService;
  var echoTargetApp;

  class EchoTarget extends Application {
    acceptConnection(url, echoSP) {
      echoTargetApp = this;
      var echoService = echoSP.requestService(EchoService);
      var echoString = "echo_target.js";
      echoService.echoString(echoString).then(function(response) {
        if (response.value != echoString)
          throw new Error("EchoTarget echoString=\"" + response.value + "\"");
        echoTargetApp.quit();
      });
    }
  }

  return EchoTarget;
});
