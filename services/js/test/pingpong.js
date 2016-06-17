#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

define("main", [
  "mojo/public/js/bindings",
  "mojo/services/public/js/application",
  "services/js/test/pingpong_service.mojom",
], function(bindings, application, pingPongServiceMojom) {

  const ProxyBindings = bindings.ProxyBindings;
  const StubBindings = bindings.StubBindings;
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
    };
    quit() {
      this.app.quit();
    }

    // This method is only used by the PingTargetURL test.
    pingTargetURL(url, count) {
      var app = this.app;
      return new Promise(function(resolve) {
        var pingTargetClient = {
          pong: function(value) {
            if (value == count) {
              pingTargetService.quit();
              resolve({ok: true});
            }
          }
        };
        var pingTargetService =
            app.shell.connectToService(url, PingPongService);
        pingTargetService.setClient(function(stub) {
          StubBindings(stub).delegate = pingTargetClient;
          app.pingClientStub = stub;
        });
        for(var i = 0; i <= count; i++)
          pingTargetService.ping(i);
      });
    }

    // This method is only used by the PingTargetService test.
    pingTargetService(pingTargetService, count) {
      var app = this.app;
      return new Promise(function(resolve) {
        var pingTargetClient = {
          pong: function(value) {
            if (value == count) {
              pingTargetService.quit();
              resolve({ok: true});
            }
          }
        };
        pingTargetService.setClient(function(stub) {
          StubBindings(stub).delegate = pingTargetClient;
          app.pingTargetStub = stub;
        });
        for(var i = 0; i <= count; i++)
          pingTargetService.ping(i);
      });
    }

    // This method is only used by the GetTargetService test.
    getPingPongService(pingPongServiceStub) {
      var impl = new PingPongServiceImpl(this);
      StubBindings(pingPongServiceStub).delegate = impl;
    }
  }

  class PingPong extends Application {
    acceptConnection(url, serviceExchange) {
      serviceExchange.provideService(
          PingPongService, PingPongServiceImpl.bind(null, this));
    }
  }

  return PingPong;
});
