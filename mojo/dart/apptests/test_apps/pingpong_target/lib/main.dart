// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

import 'package:_mojo_for_test_only/test/pingpong_service.mojom.dart';

class PingPongServiceImpl implements PingPongService {
  PingPongService _service;
  Application _application;
  PingPongClient _pingPongClient;

  PingPongServiceImpl(this._application, MojoMessagePipeEndpoint endpoint) {
    _service = new PingPongServiceInterface.fromEndpoint(endpoint, this);
  }

  PingPongServiceImpl.fromInterfaceRequest(
      PingPongServiceInterfaceRequest service) {
    service.impl = this;
    _service = service;
  }

  void setClient(PingPongClientInterface client) {
    assert(_pingPongClient == null);
    _pingPongClient = client;
  }

  void ping(int pingValue) => _pingPongClient.pong(pingValue + 1);

  // These methods are unimplemented; they merely throw on invocation.
  void pingTargetUrl(String url, int count, void callback(bool ok)) =>
      throw "Unimplemented";
  void pingTargetService(PingPongServiceInterface service, int count,
      void callback(bool ok)) =>
      throw "Unimplemented";
  void getPingPongServiceDelayed(PingPongServiceInterfaceRequest service) =>
      throw "Unimplemented";

  void getPingPongService(PingPongServiceInterfaceRequest service) {
    new PingPongServiceImpl.fromInterfaceRequest(service);
  }

  void quit() {}
}

class PingPongApplication extends Application {
  PingPongApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  @override
  void acceptConnection(String requestorUrl, String resolvedUrl,
      ApplicationConnection connection) {
    // Provide the service implemented by PingPongServiceImpl.
    connection.provideService(PingPongService.serviceName,
        (endpoint) => new PingPongServiceImpl(this, endpoint));
  }

  Future closeApplication() async {
    await close();
    MojoHandle.reportLeakedHandles();
  }
}

main(List args, Object handleToken) {
  MojoHandle appHandle = new MojoHandle(handleToken);
  new PingPongApplication.fromHandle(appHandle);
}
