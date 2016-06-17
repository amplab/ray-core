// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This Mojo app is a benchmark of Mojo/Dart IPC Round Trip Times.
// It creates many proxies from a single isolate, and makes requests using those
// proxies round-robin.
// To run it, and the other RTT benchmarks:
//
// $ ./mojo/devtools/common/mojo_benchmark [--release] mojo/tools/data/rtt_benchmarks

import 'dart:async';
import 'dart:developer';

import 'package:args/args.dart' as args;
import 'package:mojo/application.dart';
import 'package:mojo/core.dart';
import 'package:_mojo_for_test_only/mojo/examples/echo.mojom.dart';

class EchoTracingApp extends Application {
  static const Duration kWarmupDuration = const Duration(seconds: 1);
  static const Duration kDelay = const Duration(microseconds: 50);
  List<EchoProxy> _echoProxies = [];
  bool _doEcho;
  bool _warmup;
  int _numClients;
  int _numActiveClients;

  EchoTracingApp.fromHandle(MojoHandle handle) : super.fromHandle(handle) {
    onError = _errorHandler;
  }

  void initialize(List<String> arguments, String url) {
    var parser = new args.ArgParser(allowTrailingOptions: true);
    parser.addFlag('cpp-server', defaultsTo: false, negatable: false);
    parser.addOption('num-clients', defaultsTo: "1");
    parser.addOption('num-active-clients', defaultsTo: "1");
    var argResults = parser.parse(arguments);

    String echoUrl = "mojo:dart_echo_server";
    if (argResults["cpp-server"]) {
      echoUrl = "mojo:echo_server";
    }
    _numClients = int.parse(argResults["num-clients"]);
    _numActiveClients = int.parse(argResults["num-active-clients"]);

    // Setup the connection to the echo app.
    for (int i = 0; i < _numClients; i++) {
      var newProxy = new EchoProxy.unbound();
      newProxy.ctrl.errorFuture.then((e) {
        _errorHandler(e);
      });
      connectToService(echoUrl, newProxy);
      _echoProxies.add(newProxy);
    }

    // Start echoing.
    _doEcho = true;
    Timer.run(() => _run(0));

    // Begin tracing echo rtts after waiting for kWarmupDuration.
    _warmup = true;
    new Timer(kWarmupDuration, () => _warmup = false);
  }

  _run(int idx) {
    if (idx == _numActiveClients) {
      idx = 0;
    }
    if (_doEcho) {
      if (_warmup) {
        _echo(idx, "ping", (String r) {
          new Timer(kDelay, () => _run(idx + 1));
        });
      } else {
        _tracedEcho(idx, "ping", (String r) {
          new Timer(kDelay, () => _run(idx + 1));
        });
      }
    }
  }

  void _tracedEcho(int idx, String s, void callback(String r)) {
    // This is not the correct way to use the Timeline API. In particular,
    // the start and end events below could occur on different threads.
    // This could mess up the interpretation of the trace by about:tracing.
    // We do this here as a hack to reduce overhead. The correct way to do
    // this would be to emit an async begin and end pair using TimelineTask.
    // Using the sync API means that we only record one event after the finish
    // time has been recorded.
    Timeline.startSync("ping");
    _echo(idx, s, (String r) {
      Timeline.finishSync();
      callback(r);
    });
  }

  void _echo(int idx, String s, void callback(String r)) {
    _echoProxies[idx].echoString(s, callback);
  }

  _errorHandler(Object e) {
    _doEcho = false;
    return Future.wait(_echoProxies.map((p) => p.close())).then((_) {
      MojoHandle.reportLeakedHandles();
    });
  }
}

main(List args, Object handleToken) {
  MojoHandle appHandle = new MojoHandle(handleToken);
  new EchoTracingApp.fromHandle(appHandle);
}
