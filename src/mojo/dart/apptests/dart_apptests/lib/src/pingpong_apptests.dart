// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library pingpong_apptests;

import 'dart:async';

import 'package:mojo_apptest/apptest.dart';
import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:_mojo_for_test_only/test/pingpong_service.mojom.dart';

class _TestingPingPongClient extends PingPongClient {
  PingPongClientInterface client;
  Completer _completer;

  _TestingPingPongClient() {
    client = new PingPongClientInterface(this);
  }

  waitForPong() async {
    _completer = new Completer();
    return _completer.future;
  }

  void pong(int pongValue) {
    _completer.complete(pongValue);
    _completer = null;
  }
}

pingpongApptests(Application application, String url) {
  group('Ping-Pong Service Apptests', () {
    // Verify that "pingpong.dart" implements the PingPongService interface
    // and sends responses to our client.
    test('Ping Service To Pong Client', () async {
      var pingPongService = new PingPongServiceInterfaceRequest();
      pingPongService.ctrl.errorFuture.then((e) => fail('$e'));
      application.connectToService("mojo:dart_pingpong", pingPongService);

      var pingPongClient = new _TestingPingPongClient();
      pingPongService.setClient(pingPongClient.client);

      pingPongService.ping(1);
      var pongValue = await pingPongClient.waitForPong();
      expect(pongValue, equals(2));

      pingPongService.ping(100);
      pongValue = await pingPongClient.waitForPong();
      expect(pongValue, equals(101));

      await pingPongClient.client.close();
      await pingPongService.close();
    });

    // Verify that "pingpong.dart" can connect to "pingpong_target.dart", act as
    // its client, and return a Future that only resolves after the
    // target.ping() => client.pong() methods have executed 9 times.
    test('Ping Target URL', () async {
      var pingPongService = new PingPongServiceInterfaceRequest();
      pingPongService.ctrl.errorFuture.then((e) => fail('$e'));
      application.connectToService("mojo:dart_pingpong", pingPongService);

      var c = new Completer();
      pingPongService.pingTargetUrl("mojo:dart_pingpong_target", 9, (bool ok) {
        c.complete(ok);
      });
      expect(await pingPongService.responseOrError(c.future), isTrue);

      await pingPongService.close();
    });

    // Same as the previous test except that instead of providing the
    // pingpong_target.dart URL, we provide a connection to its PingPongService.
    test('Ping Target Service', () async {
      var pingPongService = new PingPongServiceInterfaceRequest();
      pingPongService.ctrl.errorFuture.then((e) => fail('$e'));
      application.connectToService("mojo:dart_pingpong", pingPongService);

      var targetService = new PingPongServiceInterfaceRequest();
      targetService.ctrl.errorFuture.then((e) => fail('$e'));
      application.connectToService("mojo:dart_pingpong_target", targetService);

      var c = new Completer();
      pingPongService.pingTargetService(targetService, 9, (bool ok) {
        c.complete(ok);
      });
      expect(await pingPongService.responseOrError(c.future), isTrue);

      // This side no longer has access to the pipe.
      expect(targetService.ctrl.isOpen, equals(false));
      expect(targetService.ctrl.isBound, equals(false));

      await pingPongService.close();
    });

    // Verify that Dart can implement an interface "request" parameter.
    test('Get Target Service', () async {
      var pingPongService = new PingPongServiceInterfaceRequest();
      pingPongService.ctrl.errorFuture.then((e) => fail('$e'));
      application.connectToService("mojo:dart_pingpong", pingPongService);

      var targetService = new PingPongServiceInterfaceRequest();
      targetService.ctrl.errorFuture.then((e) => fail('$e'));
      pingPongService.getPingPongService(targetService);

      var pingPongClient = new _TestingPingPongClient();
      targetService.setClient(pingPongClient.client);

      targetService.ping(1);
      var pongValue = await pingPongClient.waitForPong();
      expect(pongValue, equals(2));

      targetService.ping(100);
      pongValue = await pingPongClient.waitForPong();
      expect(pongValue, equals(101));

      await pingPongClient.client.close();
      await targetService.close();
      await pingPongService.close();
    });

    // Verify that Dart can implement an interface "request" parameter that
    // isn't hooked up to an actual service implementation until after some
    // delay.
    test('Get Target Service Delayed', () async {
      var pingPongService = new PingPongServiceInterfaceRequest();
      pingPongService.ctrl.errorFuture.then((e) => fail('$e'));
      application.connectToService("mojo:dart_pingpong", pingPongService);

      var targetService = new PingPongServiceInterfaceRequest();
      targetService.ctrl.errorFuture.then((e) => fail('$e'));
      pingPongService.getPingPongServiceDelayed(targetService);

      var pingPongClient = new _TestingPingPongClient();
      targetService.setClient(pingPongClient.client);

      targetService.ping(1);
      var pongValue = await pingPongClient.waitForPong();
      expect(pongValue, equals(2));

      targetService.ping(100);
      pongValue = await pingPongClient.waitForPong();
      expect(pongValue, equals(101));

      await pingPongClient.client.close();
      await targetService.close();
      await pingPongService.close();
    });
  });
}
