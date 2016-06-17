// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library mojo_apptest;

import 'dart:async';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

// Import and reexport the test package.
import 'package:test/test.dart';
export 'package:test/test.dart';

typedef AppTestFunction(Application app, String url);

// This class is an application that does nothing but tears down the connections
// between each test.
class _ConnectionToShellApplication extends Application {
  final List<AppTestFunction> _testFunctions;

  _ConnectionToShellApplication.fromHandle(
      MojoHandle handle, this._testFunctions)
      : super.fromHandle(handle);

  // Only run the test suite passed in once we have received an initialize()
  // call from the shell. We need to first have a valid connection to the shell
  // so that apptests can connect to other applications.
  void initialize(List<String> args, String url) {
    group('dart_apptests', () {
      setUp(testSetUp);
      tearDown(testTearDown);
      for (var testFunction in _testFunctions) {
        testFunction(this, url);
      }
    });

    tearDownAll(() async {
      await close();
      MojoHandle.reportLeakedHandles();
    });
  }

  void testSetUp() {}

  Future testTearDown() async {
    // Reset any connections between tests.
    await resetConnections();
  }
}

/// The public interface to apptests.
///
/// In a dart mojo application, [incomingHandle] is `args[0]`. [testFunctions]
/// is list of [AppTestFunction]. Each function will be passed the application
/// and url.
runAppTests(var incomingHandle, List<AppTestFunction> testFunctions) {
  var appHandle = new MojoHandle(incomingHandle);
  var application =
      new _ConnectionToShellApplication.fromHandle(appHandle, testFunctions);

  /// [Application]'s [initialize] will be called.
}
