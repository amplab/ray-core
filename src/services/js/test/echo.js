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
  var echoApp;

  class EchoServiceImpl {
    echoString(s) {
      return Promise.resolve({value: s});
    }

    quit() {
      echoApp.quit();
    }
  }

  class Echo extends Application {
    acceptConnection(url, serviceExchange) {
      echoApp = this;
      serviceExchange.provideService(EchoService, EchoServiceImpl);
    }
  }

  return Echo;
});
