// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library echo_apptests;

import 'dart:async';

import 'package:mojo_apptest/apptest.dart';
import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:_mojo_for_test_only/test/echo_service.mojom.dart';

class EchoServiceMock implements EchoService {
  void echoString(String value, void callback(String value)) {
    callback(value);
  }

  void delayedEchoString(String value, int millis,
                         void callback(String value)) {
    new Timer(new Duration(milliseconds : millis), () => callback(value));
  }

  void swap() {}

  void quit() {}
}

echoApptests(Application application, String url) {
  group('Echo Service Apptests', () {
    test('String', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      var c = new Completer();
      echo.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals("foo"));

      c = new Completer();
      echo.echoString("quit", (String value) {
        fail("unreachable");
      });
      try {
        await echo.responseOrError(c.future);
        fail("unreachable");
      } catch (e) {
        expect(e is ProxyError, isTrue);
      }

      await echo.close();
    });

    test('Empty String', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      var c = new Completer();
      echo.echoString("", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals(""));

      c = new Completer();
      echo.echoString("quit", (String value) {
        fail("unreachable");
      });
      try {
        await echo.responseOrError(c.future);
        fail("unreachable");
      } catch (e) {
        expect(e is ProxyError, isTrue);
      }

      await echo.close();
    });

    test('Null String', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      var c = new Completer();
      echo.echoString(null, (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals(null));

      c = new Completer();
      echo.echoString("quit", (String value) {
        fail("unreachable");
      });
      try {
        await echo.responseOrError(c.future);
        fail("unreachable");
      } catch (e) {
        expect(e is ProxyError, isTrue);
      }

      await echo.close();
    });

    test('Delayed Success', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      var milliseconds = 100;
      var watch = new Stopwatch()..start();
      var c = new Completer();
      echo.delayedEchoString("foo", milliseconds, (String value) {
        var elapsed = watch.elapsedMilliseconds;
        c.complete([value, elapsed]);
      });
      var result = await echo.responseOrError(c.future);
      expect(result[0], equals("foo"));
      expect(result[1], greaterThanOrEqualTo(milliseconds));

      c = new Completer();
      echo.echoString("quit", (String value) {
        fail("unreachable");
      });
      try {
        await echo.responseOrError(c.future);
        fail("unreachable");
      } catch (e) {
        expect(e is ProxyError, isTrue);
      }

      await echo.close();
    });

    test('Delayed Close', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      var milliseconds = 100;
      var c = new Completer();
      echo.delayedEchoString("quit", milliseconds, (String value) {
        fail("unreachable");
      });
      try {
        await echo.responseOrError(c.future);
        fail("unreachable");
      } catch (e) {
        expect(e is ProxyError, isTrue);
      };

      return new Future.delayed(
          new Duration(milliseconds: 10), () => echo.close());
    });

    test('Swap', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      for (int i = 0; i < 10; i++) {
        var c = new Completer();
        echo.echoString("foo", (String value) {
          c.complete(value);
        });
        expect(await echo.responseOrError(c.future), equals("foo"));
      }

      echo.ctrl.errorFuture.then((e) {
        fail("echo: $e");
      });

      // Trigger an implementation swap in the echo server.
      echo.swap();

      expect(echo.ctrl.isBound, isTrue);

      for (int i = 0; i < 10; i++) {
        var c = new Completer();
        echo.echoString("foo", (String value) {
          c.complete(value);
        });
        expect(await echo.responseOrError(c.future), equals("foo"));
      }

      await echo.close();
    });

    test('Multiple Error Checks Success', () {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      List<Future> futures = [];
      for (int i = 0; i < 100; i++) {
        var c = new Completer();
        echo.echoString("foo", (String value) {
          c.complete(value);
        });
        futures.add(echo.responseOrError(c.future));
      }
      return Future.wait(futures).whenComplete(() => echo.close());
    });

    test('Multiple Error Checks Fail', () {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      List<Future> futures = [];
      var milliseconds = 100;
      for (int i = 0; i < 100; i++) {
        var c = new Completer();
        echo.delayedEchoString("foo", milliseconds, (String value) {
          fail("unreachable");
        });
        var f = echo.responseOrError(c.future).then((_) {
          fail("unreachable");
        }, onError: (e) {
          expect(e is ProxyError, isTrue);
        });
        futures.add(f);
      }
      return echo.close().then((_) => Future.wait(futures));
    });

    test('Uncaught Call Closed', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      // Do a normal call.
      var c = new Completer();
      echo.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals("foo"));

      // Close the proxy.
      await echo.close();

      // Try to do another call, which should not return.
      echo.echoString("foo", (_) {
        fail('This should be unreachable');
      });
    });

    test('Catch Call Closed', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      // Do a normal call.
      var c = new Completer();
      echo.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals("foo"));

      // Close the proxy.
      await echo.close();

      // Try to do another call, which should fail.
      bool caughtException = false;
      c = new Completer();
      echo.echoString("foo", (String value) {
        fail("unreachable");
      });
      try {
        await echo.responseOrError(c.future);
        fail('This should be unreachable');
      } on ProxyError catch (e) {
        caughtException = true;
      }
      expect(caughtException, isTrue);
    });

    test('Catch Call Sequence Closed Twice', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      // Do a normal call.
      var c = new Completer();
      echo.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals("foo"));

      // Close the proxy.
      await echo.close();

      // Try to do another call, which should fail.
      bool caughtException = false;
      c = new Completer();
      echo.echoString("foo", (String value) {
        fail("unreachable");
      });
      try {
        await echo.responseOrError(c.future);
        fail('This should be unreachable');
      } on ProxyError catch (e) {
        caughtException = true;
      }
      expect(caughtException, isTrue);

      // Try to do another call, which should fail.
      caughtException = false;
      c = new Completer();
      echo.echoString("foo", (String value) {
        fail("unreachable");
      });
      try {
        await echo.responseOrError(c.future);
        fail('This should be unreachable');
      } on ProxyError catch (e) {
        caughtException = true;
      }
      expect(caughtException, isTrue);
    });

    test('Catch Call Parallel Closed Twice', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      // Do a normal call.
      var c = new Completer();
      echo.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals("foo"));

      // Close the proxy.
      await echo.close();

      // Queue up two calls after the close, and make sure they both fail.
      var c1 = new Completer();
      echo.echoString("foo", (String value) {
        fail("unreachable");
      });
      var c2 = new Completer();
      echo.echoString("foo", (String value) {
        fail("unreachable");
      });
      var f1 = echo.responseOrError(c1.future).then((_) {
        fail("unreachable");
      }, onError: (e) {
        expect(e is ProxyError, isTrue);
      });
      var f2 = echo.responseOrError(c2.future).then((_) {
        fail("unreachable");
      }, onError: (e) {
        expect(e is ProxyError, isTrue);
      });

      return Future.wait([f1, f2]);
    });

    test('Unbind, Rebind, Close', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      // Do a normal call.
      var c = new Completer();
      echo.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals("foo"));

      var endpoint = echo.ctrl.unbind();
      echo.ctrl.bind(endpoint);

      c = new Completer();
      echo.echoString("quit", (String value) {
        fail("unreachable");
      });
      try {
        await echo.responseOrError(c.future);
        fail("unreachable");
      } catch (e) {
        expect(e is ProxyError, isTrue);
      }

      await echo.close();
    });

    test('Unbind, rebind to same', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      // Do a normal call.
      var c = new Completer();
      echo.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals("foo"));

      var endpoint = echo.ctrl.unbind();
      echo.ctrl.bind(endpoint);

      // Do a normal call.
      c = new Completer();
      echo.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals("foo"));

      c = new Completer();
      echo.echoString("quit", (String value) {
        fail("unreachable");
      });
      try {
        await echo.responseOrError(c.future);
        fail("unreachable");
      } catch (e) {
        expect(e is ProxyError, isTrue);
      }

      await echo.close();
    });

    test('Unbind, rebind to different', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      // Do a normal call.
      var c = new Completer();
      echo.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals("foo"));

      var endpoint = echo.ctrl.unbind();
      var differentEchoProxy = new EchoServiceProxy.fromEndpoint(endpoint);

      // Do a normal call.
      c = new Completer();
      differentEchoProxy.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await differentEchoProxy.responseOrError(c.future), equals("foo"));

      await differentEchoProxy.close();
    });

    test('Unbind, rebind to different, close original', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      // Do a normal call.
      var c = new Completer();
      echo.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals("foo"));

      var endpoint = echo.ctrl.unbind();
      var differentEchoProxy = new EchoServiceProxy.fromEndpoint(endpoint);
      await echo.close();

      // Do a normal call.
      c = new Completer();
      differentEchoProxy.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await differentEchoProxy.responseOrError(c.future), equals("foo"));

      await differentEchoProxy.close();
    });

    test('Mock', () async {
      var echo = new EchoServiceInterface.fromMock(new EchoServiceMock());

      var c = new Completer();
      echo.echoString("foo", (String value) {
        c.complete(value);
      });
      expect(await echo.responseOrError(c.future), equals("foo"));

      await echo.close();
    });

    test('Right Zone', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      await runZoned(() async {
        var c = new Completer();
        var currentZone = Zone.current;
        var callbackZone = null;
        echo.echoString("foo", (String value) {
          callbackZone = Zone.current;
          c.complete(value);
        });
        expect(await echo.responseOrError(c.future), equals("foo"));
        expect(identical(currentZone, callbackZone), isTrue);
        expect(identical(callbackZone, Zone.ROOT), isFalse);
      });

      await echo.close();
    });

    test('Zone Error', () async {
      var echo = EchoService.connectToService(application, "mojo:dart_echo");

      echo.ctrl.onError = ((e) {
        fail("Error incorrectly passed to onError: $e");
      });

      var c = new Completer();
      await runZoned(() async {
        echo.echoString("foo", (String value) {
          throw "An error";
        });
      }, onError: (e) {
        expect(e, equals("An error"));
        c.complete(null);
      });

      await c.future;
      await echo.close();
    });
  });
}
