#!mojo mojo:dart_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Run with, e.g.:
// mojo_shell "mojo:dart_wget http://www.google.com"

import 'dart:async';
import 'dart:typed_data';

import 'package:mojo/application.dart';
import 'package:mojo/core.dart';
import 'package:mojo/mojo/url_request.mojom.dart';
import 'package:mojo/mojo/url_response.mojom.dart';
import 'package:mojo_services/mojo/network_service.mojom.dart';
import 'package:mojo_services/mojo/url_loader.mojom.dart';

class WGet extends Application {
  NetworkServiceInterfaceRequest _networkService;
  UrlLoaderProxy _urlLoader;

  WGet.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void initialize(List<String> args, String url) {
    run(args);
  }

  run(List<String> args) async {
    if (args == null || args.length != 2) {
      throw "Expected URL argument";
    }

    ByteData bodyData = await _getUrl(args[1]);
    print(">>> Body <<<");
    print(new String.fromCharCodes(bodyData.buffer.asUint8List()));
    print(">>> EOF <<<");

    _closeInterfaces();
    await close();
    MojoHandle.reportLeakedHandles();
  }

  Future<ByteData> _getUrl(String url) async {
    _initInterfacesIfNeeded();

    var urlRequest = new UrlRequest()
      ..url = url
      ..autoFollowRedirects = true;

    var c = new Completer();
    _urlLoader.start(urlRequest, (UrlResponse response) {
      c.complete(response);
    });
    var urlResponse = await _urlLoader.responseOrError(c.future);
    print(">>> Headers <<<");
    print(urlResponse.headers.join('\n'));

    return DataPipeDrainer.drainHandle(urlResponse.body);
  }

  void _initInterfacesIfNeeded() {
    if (_networkService == null) {
      _networkService = new NetworkServiceInterfaceRequest();
      connectToService("mojo:network_service", _networkService);
    }
    if (_urlLoader == null) {
      _urlLoader = new UrlLoaderInterfaceRequest();
      _networkService.createUrlLoader(_urlLoader);
    }
  }

  void _closeInterfaces() {
    _urlLoader.close();
    _urlLoader = null;
    _networkService.close();
    _networkService = null;
  }
}

main(List args, handleToken) {
  MojoHandle appHandle = new MojoHandle(handleToken);
  new WGet.fromHandle(appHandle);
}
