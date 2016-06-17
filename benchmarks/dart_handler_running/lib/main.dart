#!mojo mojo:dart_content_handler
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This benchmark helper app allows to measure initialization time of a dart
// app being connected to when the dart content handler is already running. As
// the helper itself is a dart app, we know that the dart content handler is
// running when we issue the call to connectToApplication.

import 'dart:async';
import 'dart:developer';

import 'package:mojo/application.dart';
import 'package:mojo/core.dart';

class DartHandlerRunning extends Application {
  DartHandlerRunning.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  Future initialize(List<String> args, String url) async {
    Timeline.instantSync("connecting");
    ApplicationConnection connection =
        connectToApplication("mojo:dart_startup");
  }
}

main(List args, Object handleToken) {
  MojoHandle appHandle = new MojoHandle(handleToken);
  new DartHandlerRunning.fromHandle(appHandle);
}
