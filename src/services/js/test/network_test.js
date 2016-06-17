#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

define("main", [
  "mojo/public/interfaces/network/url_request.mojom",
  "mojo/public/js/core",
  "mojo/services/network/interfaces/network_service.mojom",
  "mojo/services/public/js/application",
  "services/js/test/network_test_service.mojom",
], function(urlRequestMojom,
            core,
            networkServiceMojom,
            application,
            networkTestServiceMojom) {

  const Application = application.Application;
  const NetworkService = networkServiceMojom.NetworkService;
  const URLRequest = urlRequestMojom.URLRequest;
  const NetworkTestService = networkTestServiceMojom.NetworkTestService;

  class NetworkTestServiceImpl {
    constructor(app) {
      this.app = app;
      var netService = app.shell.connectToService(
          "mojo:network_service", NetworkService);
      var urlLoader;
      netService.createURLLoader(function(x){urlLoader = x;});
      this.urlLoader = urlLoader;
    }

    getFileSize(url) {
      var impl = this;
      return new Promise(function(resolve) {
        var urlRequest = new URLRequest({
          url: url,
          method: "GET",
          auto_follow_redirects: true
        });
        impl.urlLoader.start(urlRequest).then(function(result) {
          core.drainData(result.response.body).then(
            function(result) {
              resolve({ok: true, size: result.buffer.byteLength});
            });
        }).catch(function() {
          resolve({ok: false});
        });
      });
    }

    quit() {
      this.app.quit();
    }
  }

  class NetworkTest extends Application {
    acceptConnection(url, serviceExchange) {
      serviceExchange.provideService(
          NetworkTestService,
          NetworkTestServiceImpl.bind(null, this));
    }
  }

  return NetworkTest;
});
