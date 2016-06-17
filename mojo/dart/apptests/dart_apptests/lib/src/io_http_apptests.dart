// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library io_http_apptests;

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:mojo_apptest/apptest.dart';
import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

tests(Application application, String url) {
  group('Http Apptests', () {
    // Bind server to a random port.
    test('Http Server Bind', () async {
      var server = await HttpServer.bind(InternetAddress.LOOPBACK_IP_V4, 0);
      expect(server, isNotNull);
      expect(server.port, greaterThan(1024));
      await server.close();
    });
    // Bind server to a random port.
    // Make a request.
    // Expect a 404 response.
    test('Http Server Request 404', () async {
      var server = await HttpServer.bind(InternetAddress.LOOPBACK_IP_V4, 0);
      expect(server, isNotNull);
      expect(server.port, greaterThan(1024));
      server.listen((HttpRequest request) {
        request.response.statusCode = HttpStatus.NOT_FOUND;
        request.response.close();
      });
      var client = new HttpClient();
      expect(client, isNotNull);
      var request = await client.get("127.0.0.1", server.port, "fox.txt");
      expect(request, isNotNull);
      var response = await request.close();
      expect(response, isNotNull);
      expect(response.statusCode, equals(HttpStatus.NOT_FOUND));
      await server.close();
    });
    // Bind server to a random port.
    // Make a request.
    // Expect an OK response.
    test('Http Server Response', () async {
    var server = await HttpServer.bind(InternetAddress.LOOPBACK_IP_V4, 0);
      expect(server, isNotNull);
      expect(server.port, greaterThan(1024));
      server.listen((HttpRequest request) {
        expect(request.uri.path, equals('/bar.txt'));
        expect(request.method, equals('GET'));
        request.response.write('contents of bar.txt');
        request.response.close();
      });
      var client = new HttpClient();
      expect(client, isNotNull);
      var request = await client.get("127.0.0.1", server.port, 'bar.txt');
      expect(request, isNotNull);
      var response = await request.close();
      expect(response, isNotNull);
      expect(response.statusCode, equals(HttpStatus.OK));
      // Reduce to a single list.
      var payload = await response.reduce((p, e) => p..addAll(e));
      // Verify payload is correct.
      expect('contents of bar.txt', equals(UTF8.decode(payload)));
      await server.close();
    });
  });
}
