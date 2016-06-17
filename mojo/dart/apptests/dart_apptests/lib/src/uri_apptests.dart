// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library uri_apptests;

import 'dart:async';
import 'dart:isolate';

import 'package:mojo_apptest/apptest.dart';
import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

void checkBaseUri(String uriBaseAsString) {
  Uri uri = Uri.parse(uriBaseAsString);
  expect(uri.path.endsWith('dart_apptests.mojo'), isTrue);
}

void getBaseUri(sendPort) {
  assert(sendPort != null);
  String uriBaseAsString = Uri.base.toString();
  sendPort.send(uriBaseAsString);
}

tests(Application application, String url) {
  group('Uri Apptests', () {
    test('Uri.base', () {
      String uriBaseAsString = Uri.base.toString();
      checkBaseUri(uriBaseAsString);
    });
    test('Isolate.spawn Uri.base', () async {
      // Create a RawReceivePort and Completer to wait on the value of
      // Uri.base.toString() from the child isolate.
      RawReceivePort rp = new RawReceivePort();
      Completer completer = new Completer();
      rp.handler = (String uriBaseAsString) {
        rp.close();
        completer.complete(uriBaseAsString);
      };
      Isolate childIso =
          await Isolate.spawn(getBaseUri, rp.sendPort);
      String uriBaseAsString = await completer.future;
      checkBaseUri(uriBaseAsString);
    });
  });
}
