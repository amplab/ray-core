// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:_mojo_for_test_only/test/echo_service.mojom.dart';

class EchoServiceImpl implements EchoService {
  EchoServiceInterface _service;
  Application _application;

  EchoServiceImpl(this._application, MojoMessagePipeEndpoint endpoint) {
    _service = new EchoServiceInterface.fromEndpoint(endpoint, this);
  }

  void echoString(String value, void callback(String value)) {
    if (value == "quit") {
      _service.close();
    }
    callback(value);
  }

  void delayedEchoString(
      String value, int millis, void callback(String value)) {
    if (value == "quit") {
      _service.close();
    }
    new Future.delayed(
        new Duration(milliseconds: millis), () => callback(value));
  }

  void swap() {
    _swapImpls(this);
  }

  void quit() {}

  static void _swapImpls(EchoServiceImpl impl) {
    final stub = impl._service;
    final app = impl._application;
    // It is not allowed to do an unbind in the midst of handling an event, so
    // it is delayed until popping back out to the event loop.
    Timer.run(() {
      final endpoint = stub.ctrl.unbind();
      new EchoServiceImpl(app, endpoint);
    });
  }
}

class EchoApplication extends Application {
  EchoApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  @override
  void acceptConnection(String requestorUrl, String resolvedUrl,
      ApplicationConnection connection) {
    connection.provideService(EchoService.serviceName,
        (endpoint) => new EchoServiceImpl(this, endpoint));
  }
}

main(List args, Object handleToken) {
  MojoHandle appHandle = new MojoHandle(handleToken);
  new EchoApplication.fromHandle(appHandle)
    ..onError = ((_) {
      MojoHandle.reportLeakedHandles();
    });
}
