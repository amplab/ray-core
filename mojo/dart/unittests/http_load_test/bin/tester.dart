// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library http_load_test;

import 'dart:async';
import 'dart:convert';
import 'dart:io';

class Launcher {
  /// Launch [executable] with [arguments]. Returns a [String] containing
  /// the standard output from the launched executable.
  static Future<String> launch(String executable,
                               List<String> arguments) async {
    var process = await Process.start(executable, arguments);

    // Completer completes once the child process exits.
    var completer = new Completer();
    String output = '';
    process.stdout.transform(UTF8.decoder)
                  .transform(new LineSplitter()).listen((line) {
      output = '$output\n$line';
      print(line);
    });
    process.stderr.transform(UTF8.decoder)
                  .transform(new LineSplitter()).listen((line) {
      output = '$output\n$line';
      print(line);
    });
    process.exitCode.then((ec) {
      output = '$output\nEXIT_CODE=$ec\n';
      completer.complete(output);
    });
    return completer.future;
  }
}

main(List<String> args) async {
  var mojo_shell_executable = args[0];
  var directory = args[1];

  HttpServer server = await HttpServer.bind('127.0.0.1', 0);

  server.listen((HttpRequest request) async {
    final String path = request.uri.toFilePath();
    final File file = new File('${directory}/${path}');
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

  var launchUrl = 'http://127.0.0.1:${server.port}/lib/main.dart';
  var output = await Launcher.launch(mojo_shell_executable, [launchUrl]);

  server.close();

  if (output.contains("ERROR")) {
    throw "test failed.";
  }
  if (!output.contains("\nEXIT_CODE=0\n")) {
    throw "Test failed.";
  }
  if (!output.contains("\nPASS")) {
    throw "Test failed.";
  }
}
