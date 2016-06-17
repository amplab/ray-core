#!mojo mojo:dart_content_handler
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:mojo_services/mojo/device_info.mojom.dart';

class DeviceInfoApp extends Application {
  final DeviceInfoProxy _deviceInfo = new DeviceInfoProxy.unbound();

  DeviceInfoApp.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  Future initialize(List<String> args, String url) async {
    connectToService("mojo:device_info", _deviceInfo);
    var c = new Completer();
    _deviceInfo.getDeviceType((DeviceInfoDeviceType type) {
      c.complete(type);
    });
    print(await _deviceInfo.responseOrError(c.future));
    _deviceInfo.close();
    close();
  }
}

main(List args, Object handleToken) {
  MojoHandle appHandle = new MojoHandle(handleToken);
  new DeviceInfoApp.fromHandle(appHandle);
}
