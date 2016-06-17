#!mojo mojo:dart_content_handler
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// To run this app:
// mojo_shell mojo:hello

import 'dart:async';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

class Hello extends Application {
  Hello.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void initialize(List<String> args, String url) {
    print("$url Hello");

    var worldUrl = url.replaceAll("hello", "world");
    print("Connecting to $worldUrl");

    // We expect to find the world mojo application at the same
    // path as this application.
    var c = connectToApplication(worldUrl);

    // A call to close() here would cause this app to go down before the "world"
    // app has a chance to come up. Instead, we wait to close this app until
    // the "world" app comes up, does its print, and closes its end of the
    // connection.
    c.remoteServiceProvider.ctrl.onError = closeApplication;
  }

  Future closeApplication(e) async {
    await close();
    MojoHandle.reportLeakedHandles();
  }
}

main(List args, Object handleToken) {
  MojoHandle appHandle = new MojoHandle(handleToken);
  new Hello.fromHandle(appHandle);
}
