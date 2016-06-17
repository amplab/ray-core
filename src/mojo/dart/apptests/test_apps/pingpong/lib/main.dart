// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

import 'package:_mojo_for_test_only/test/pingpong_service.mojom.dart';

class PingPongClientImpl implements PingPongClient {
  PingPongClientInterface client;
  Completer _completer;
  int _count;

  PingPongClientImpl(this._count, this._completer) {
    client = new PingPongClientInterface(this);
  }

  void pong(int pongValue) {
    if (pongValue == _count) {
      _completer.complete(null);
      client.close();
    }
  }
}

class PingPongServiceImpl implements PingPongService {
  PingPongService _service;
  Application _application;
  PingPongClient _pingPongClient;

  PingPongServiceImpl(this._application, MojoMessagePipeEndpoint endpoint) {
    _service = new PingPongServiceInterface.fromEndpoint(endpoint, this);
  }

  void setClient(PingPongClientInterface client) {
    assert(_pingPongClient == null);
    _pingPongClient = client;
  }

  void ping(int pingValue) {
    if (_pingPongClient != null) {
      _pingPongClient.pong(pingValue + 1);
    }
  }

  void pingTargetUrl(String url, int count, void callback(bool ok)) {
    if (_application == null) {
      callback(false);
      return;
    }
    var completer = new Completer();
    var pingPongService = new PingPongServiceInterfaceRequest();
    _application.connectToService(url, pingPongService);

    var pingPongClient = new PingPongClientImpl(count, completer);
    pingPongService.setClient(pingPongClient.client);

    for (var i = 0; i < count; i++) {
      pingPongService.ping(i);
    }
    completer.future.then((_) {
      callback(true);
      pingPongService.close();
    });
  }

  void pingTargetService(
      PingPongServiceInterface service, int count, void callback(bool ok)) {
    var pingPongService = service;
    var completer = new Completer();
    var client = new PingPongClientImpl(count, completer);
    pingPongService.setClient(client.client);

    for (var i = 0; i < count; i++) {
      pingPongService.ping(i);
    }
    completer.future.then((_) {
      callback(true);
      pingPongService.close();
    });
  }

  getPingPongService(PingPongServiceInterfaceRequest service) {
    var targetService = new PingPongServiceInterfaceRequest();
    _application.connectToService("mojo:dart_pingpong_target", targetService);

    // Pass along the interface request to another implementation of the
    // service.
    targetService.getPingPongService(service);
    targetService.close();
  }

  getPingPongServiceDelayed(PingPongServiceInterfaceRequest service) {
    Timer.run(() {
      var endpoint = service.ctrl.unbind();
      new Timer(const Duration(milliseconds: 10), () {
        var targetService = new PingPongServiceInterfaceRequest();
        _application.connectToService(
            "mojo:dart_pingpong_target", targetService);

        // Pass along the interface request to another implementation of the
        // service.
        service.ctrl.bind(endpoint);
        targetService.getPingPongService(service);
        targetService.close();
      });
    });
  }

  void quit() {}
}

class PingPongApplication extends Application {
  PingPongApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  @override
  void acceptConnection(String requestorUrl, String resolvedUrl,
      ApplicationConnection connection) {
    connection.provideService(PingPongService.serviceName,
        (endpoint) => new PingPongServiceImpl(this, endpoint),
        description: PingPongService.serviceDescription);
  }
}

main(List args, Object handleToken) {
  MojoHandle appHandle = new MojoHandle(handleToken);
  new PingPongApplication.fromHandle(appHandle)
    ..onError = ((_) {
      MojoHandle.reportLeakedHandles();
    });
}
