// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import "package:args/args.dart";
import "package:path/path.dart" as path;
import "package:test/test.dart";

void main(List<String> args) {
  final ArgParser parser = new ArgParser();
  parser.addOption('mojo-shell',
      help: 'Path to the Mojo shell.');
  final ArgResults results = parser.parse(args);
  final String shellPath = results['mojo-shell'];
  final List<String> devtoolsFlags = results.rest;

  HttpServer testServer;
  String testServerUrl;
  setUp(() async {
    testServer = await HttpServer.bind('127.0.0.1', 0);
    testServer.listen((HttpRequest request) {
      request.response.write('Hello, world!');
      request.response.close();
    });
    testServerUrl = 'http://127.0.0.1:${testServer.port}/';
  });

  tearDown(() async {
    testServer.close();
  });

  test("Wget .mojo", () async {
    final String mojoUrl = 'mojo:dart_wget';
    final List<String> args = new List.from(devtoolsFlags)..add(mojoUrl);
    args.add('--args-for=$mojoUrl $testServerUrl');

    final ProcessResult result = await Process.run(shellPath, args);
    expect(result.exitCode, equals(0));
    expect(result.stdout.contains('Hello, world!'), isTrue);
  });

  test("Wget .dart", () async {
    final HttpServer server = await HttpServer.bind('127.0.0.1', 0);
    final String scriptOrigin = path.normalize(
        path.join(path.dirname(Platform.script.path), '..', '..'));
    final String packageOrigin = path.normalize(
        path.join(path.fromUri(Platform.packageRoot), '..'));

    server.listen((HttpRequest request) async {
      final String requestPath = request.uri.toFilePath();

      String origin;
      if (requestPath.startsWith('/packages')) {
        origin = packageOrigin;
      } else {
        origin = scriptOrigin;
      }

      final String filePath =
          path.join(origin, path.relative(requestPath, from: '/'));;
      final File file = new File(filePath);
      if (await file.exists()) {
        try {
          await file.openRead().pipe(request.response);
        } catch (e) {
          print(e);
        }
      } else {
        request.response.statusCode = HttpStatus.NOT_FOUND;
        request.response.close();
      }
    });

    final String dartUrl =
        'http://127.0.0.1:${server.port}/wget/lib/main.dart';

    final List<String> args = new List.from(devtoolsFlags)..add(dartUrl);
    args.add('--args-for=$dartUrl $testServerUrl');

    final ProcessResult result = await Process.run(shellPath, args);
    expect(result.exitCode, equals(0));
    expect(result.stdout.contains('Hello, world!'), isTrue);

    server.close();
  });
}
