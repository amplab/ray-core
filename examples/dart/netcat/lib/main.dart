#!mojo mojo:dart_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:core';
import 'dart:typed_data';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:mojo/mojo/network_error.mojom.dart';
import 'package:mojo_services/mojo/files/file.mojom.dart' as files;
import 'package:mojo_services/mojo/files/types.mojom.dart' as files;
import 'package:mojo_services/mojo/net_address.mojom.dart';
import 'package:mojo_services/mojo/network_service.mojom.dart';
import 'package:mojo_services/mojo/tcp_bound_socket.mojom.dart';
import 'package:mojo_services/mojo/tcp_connected_socket.mojom.dart';
import 'package:mojo_services/mojo/terminal/terminal_client.mojom.dart';

void ignoreFuture(Future f) {
  f.catchError((e) {});
}

NetAddress makeIPv4NetAddress(List<int> addr, int port) {
  var rv = new NetAddress();
  rv.family = NetAddressFamily.ipv4;
  rv.ipv4 = new NetAddressIPv4();
  rv.ipv4.addr = new List<int>.from(addr);
  rv.ipv4.port = port;
  return rv;
}

void fputs(files.File f, String s) {
  f.write((s + '\n').codeUnits, 0, files.Whence.fromCurrent, (e, n) {});
}

// Connects the terminal |File| and the socket.
// TODO(vtl):
// * Error handling: both connection/socket errors and terminal errors.
// * Relatedly, we should listen for _socketSender's peer being closed (also
//   _socket, I guess).
// * Handle the socket send pipe being full (currently, we assume it's never
//   full).
class Connector {
  final Application _application;
  files.FileProxy _terminal;
  TcpConnectedSocketProxy _socket;
  MojoDataPipeProducer _socketSender;
  MojoDataPipeConsumer _socketReceiver;
  MojoEventSubscription _socketReceiverEventSubscription;
  final ByteData _readBuffer;
  final ByteData _writeBuffer;

  // TODO(vtl): Don't just hard-code buffer sizes.
  Connector(this._application, this._terminal)
      : _readBuffer = new ByteData(16 * 1024),
        _writeBuffer = new ByteData(16 * 1024);

  Future connect(NetAddress remote_address) async {
    try {
      var networkService = new NetworkServiceProxy.unbound();
      _application.connectToService('mojo:network_service', networkService);

      NetAddress local_address = makeIPv4NetAddress([0, 0, 0, 0], 0);
      var boundSocket = new TcpBoundSocketProxy.unbound();
      var c = new Completer();
      networkService.createTcpBoundSocket(local_address, boundSocket, (_) {
        c.complete(null);
      });
      await networkService.responseOrError(c.future);
      await networkService.close();

      var sendDataPipe = new MojoDataPipe();
      _socketSender = sendDataPipe.producer;
      var receiveDataPipe = new MojoDataPipe();
      _socketReceiver = receiveDataPipe.consumer;
      _socket = new TcpConnectedSocketProxy.unbound();
      c = new Completer();
      boundSocket.connect(remote_address, sendDataPipe.consumer,
          receiveDataPipe.producer, _socket, (_) {
        c.complete(null);
      });
      await boundSocket.responseOrError(c.future);
      await boundSocket.close();

      // Set up reading from the terminal.
      _startReadingFromTerminal();

      // Set up reading from the socket.
      _socketReceiverEventSubscription =
          new MojoEventSubscription(_socketReceiver.handle);
      _socketReceiverEventSubscription.subscribe(_onSocketReceiverEvent);
    } catch (e) {
      _shutDown();
    }
  }

  void _startReadingFromTerminal() {
    // TODO(vtl): Do we have to do something on error?
    var c = new Completer();
    _terminal.read(
        _writeBuffer.lengthInBytes, 0,
        files.Whence.fromCurrent, (error, bytes) {
      _onReadFromTerminal(error, bytes);
      c.complete(null);
    });
    _terminal.responseOrError(c.future).catchError((_) {
      _shutDown();
    });
  }

  void _onReadFromTerminal(files.Error error, List<int> bytesRead) {
    if (error != files.Error.ok) {
      // TODO(vtl): Do terminal errors.
      return;
    }

    // TODO(vtl): Verify that |bytesRead.length| is within the expected range.
    for (var i = 0, j = 0; i < bytesRead.length; i++, j++) {
      // TODO(vtl): Temporary hack: Translate \r to \n, since we don't have
      // built-in support for that.
      if (bytesRead[i] == 13) {
        _writeBuffer.setUint8(i, 10);
      } else {
        _writeBuffer.setUint8(i, bytesRead[i]);
      }
    }

    // TODO(vtl): Handle the send data pipe being full (or closed).
    _socketSender
        .write(new ByteData.view(_writeBuffer.buffer, 0, bytesRead.length));

    _startReadingFromTerminal();
  }

  void _onSocketReceiverEvent(int mojoSignals) {
    var shouldShutDown = false;
    if (MojoHandleSignals.isReadable(mojoSignals)) {
      var numBytesRead = _socketReceiver.read(_readBuffer);
      if (_socketReceiver.status == MojoResult.kOk) {
        assert(numBytesRead > 0);
        var c = new Completer();
        _terminal.write(_readBuffer.buffer.asUint8List(0, numBytesRead), 0,
            files.Whence.fromCurrent, (e, n) {
          c.complete(null);
        });
        _terminal.responseOrError(c.future).catchError((_) {
          _shutDown();
        });
        _socketReceiverEventSubscription.enableReadEvents();
      } else {
        shouldShutDown = true;
      }
    } else if (MojoHandleSignals.isPeerClosed(mojoSignals)) {
      shouldShutDown = true;
    } else {
      String signals = MojoHandleSignals.string(mojoSignals);
      throw 'Unexpected handle event: $signals';
    }
    if (shouldShutDown) {
      _shutDown();
    }
  }

  void _shutDown() {
    if (_socketReceiverEventSubscription != null) {
      ignoreFuture(_socketReceiverEventSubscription.close());
      _socketReceiverEventSubscription = null;
    }
    if (_socketSender != null) {
      if (_socketSender.handle.isValid) _socketSender.handle.close();
      _socketSender = null;
    }
    if (_socketReceiver != null) {
      if (_socketReceiver.handle.isValid) _socketReceiver.handle.close();
      _socketReceiver = null;
    }
    if (_terminal != null) {
      ignoreFuture(_terminal.close());
      _terminal = null;
    }
  }
}

class TerminalClientImpl implements TerminalClient {
  TerminalClientStub _stub;
  Application _application;
  String _resolvedUrl;

  TerminalClientImpl(
      this._application, this._resolvedUrl, MojoMessagePipeEndpoint endpoint) {
    _stub = new TerminalClientStub.fromEndpoint(endpoint, this);
  }

  @override
  void connectToTerminal(files.FileProxy terminal) {
    var url = Uri.parse(_resolvedUrl);
    NetAddress remote_address;
    try {
      remote_address = _getNetAddressFromUrl(url);
    } catch (e) {
      fputs(
          terminal,
          'HALP: Add a query: ?host=<host>&port=<port>\n'
              '(<host> must be "localhost" or n1.n2.n3.n4)\n\n'
              'Got query parameters:\n' +
              url.queryParameters.toString());
      ignoreFuture(terminal.close());
      return;
    }

    // TODO(vtl): Currently, we only do IPv4, so this should work.
    fputs(
        terminal,
        'Connecting to: ' +
            remote_address.ipv4.addr.join('.') +
            ':' +
            remote_address.ipv4.port.toString() +
            '...');

    var connector = new Connector(_application, terminal);
    // TODO(vtl): Do we have to do something on error?
    connector.connect(remote_address).catchError((e) {});
  }

  // Note: May throw all sorts of things.
  static NetAddress _getNetAddressFromUrl(Uri url) {
    var params = url.queryParameters;
    var host = params['host'];
    return makeIPv4NetAddress(
        (host == 'localhost') ? [127, 0, 0, 1] : Uri.parseIPv4Address(host),
        int.parse(params['port']));
  }
}

class NetcatApplication extends Application {
  NetcatApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  @override
  void acceptConnection(String requestorUrl, String resolvedUrl,
      ApplicationConnection connection) {
    connection.provideService(TerminalClient.serviceName,
        (endpoint) => new TerminalClientImpl(this, resolvedUrl, endpoint));
  }
}

main(List args, Object handleToken) {
  MojoHandle appHandle = new MojoHandle(handleToken);
  new NetcatApplication.fromHandle(appHandle)
    ..onError = ((Object e) {
      MojoHandle.reportLeakedHandles();
    });
}
