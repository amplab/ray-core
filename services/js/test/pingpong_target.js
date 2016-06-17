#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

define("main", [
  "mojo/services/public/js/application",
  "services/js/test/pingpong_service.mojom",
], function(application, pingPongServiceMojom) {

  const Application = application.Application;
  const PingPongService = pingPongServiceMojom.PingPongService;

  class PingPongServiceImpl {
    constructor(app) {
      this.app = app;
    }
    setClient(client) {
      this.client = client;
    }
    ping(value) {
      this.client.pong(value + 1);
    }
    quit() {
      this.app.quit();
    }
  }

  class PingPongTarget extends Application {
    acceptConnection(url, serviceExchange) {
      serviceExchange.provideService(
          PingPongService, PingPongServiceImpl.bind(null, this));
    }
  }

  return PingPongTarget;
});
