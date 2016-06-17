// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:_mojo_for_test_only/mojo/examples/echo.mojom.dart';

class EchoImpl implements Echo {
  EchoInterface _echo;
  EchoApplication _application;

  EchoImpl(this._application, MojoMessagePipeEndpoint endpoint) {
    _echo = new EchoInterface.fromEndpoint(endpoint, this);
    _echo.ctrl.onError = _errorHandler;
  }

  void echoString(String value, void respond(String response)) {
    respond(value);
  }

  Future close() => _echo.close();

  _errorHandler(Object e) => _application.removeService(this);
}

class EchoApplication extends Application {
  List<EchoImpl> _echoServices;
  bool _closing;

  EchoApplication.fromHandle(MojoHandle handle)
      : _closing = false,
        _echoServices = [],
        super.fromHandle(handle) {
    onError = _errorHandler;
  }

  @override
  void acceptConnection(String requestorUrl, String resolvedUrl,
      ApplicationConnection connection) {
    connection.provideService(Echo.serviceName, _createService);
  }

  void removeService(EchoImpl service) {
    if (!_closing) {
      _echoServices.remove(service);
    }
  }

  EchoImpl _createService(MojoMessagePipeEndpoint endpoint) {
    if (_closing) {
      endpoint.close();
      return null;
    }
    var echoService = new EchoImpl(this, endpoint);
    _echoServices.add(echoService);
    return echoService;
  }

  _errorHandler(Object e) async {
    _closing = true;
    for (var service in _echoServices) {
      await service.close();
    }
    MojoHandle.reportLeakedHandles();
  }
}

main(List args, Object handleToken) {
  MojoHandle appHandle = new MojoHandle(handleToken);
  new EchoApplication.fromHandle(appHandle);
}
