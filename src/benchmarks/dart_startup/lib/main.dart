#!mojo mojo:dart_content_handler
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:developer';

import 'package:mojo/application.dart';
import 'package:mojo/core.dart';

class StartupBenchmarkApp extends Application {
  StartupBenchmarkApp.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void initialize(List<String> args, String url) {
    Timeline.instantSync("initialized");
  }

  @override
  void acceptConnection(String requestorUrl, String resolvedUrl,
      ApplicationConnection connection) {
    Timeline.instantSync("connected");
  }
}

main(List args, Object handleToken) {
  MojoHandle appHandle = new MojoHandle(handleToken);
  new StartupBenchmarkApp.fromHandle(appHandle);
}
