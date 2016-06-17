// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:convert';
import 'dart:isolate';
import 'dart:mojo.builtin' as builtin;
import 'dart:typed_data';

import 'package:_mojo_for_test_only/validation_test_input_parser.dart' as parser;
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:_mojo_for_test_only/mojo/test/validation_test_interfaces.mojom.dart';

class ConformanceTestInterfaceImpl implements ConformanceTestInterface {
  ConformanceTestInterfaceStub _stub;
  Completer _completer;

  ConformanceTestInterfaceImpl(
      this._completer, MojoMessagePipeEndpoint endpoint) {
    _stub = new ConformanceTestInterfaceStub.fromEndpoint(endpoint, this);
  }

  set onError(Function f) {
    _stub.ctrl.onError = f;
  }

  void _complete() => _completer.complete(null);

  method0(double param0) => _complete();
  method1(StructA param0) => _complete();
  method2(StructB param0, StructA param1) => _complete();
  method3(List<bool> param0) => _complete();
  method4(StructC param0, List<int> param1) => _complete();
  method5(StructE param0, MojoDataPipeProducer param1) {
    param1.handle.close();
    param0.dataPipeConsumer.handle.close();
    param0.structD.messagePipes.forEach((h) => h.close());
    _complete();
  }

  method6(List<List<int>> param0) => _complete();
  method7(StructF param0, List<List<int>> param1) => _complete();
  method8(List<List<String>> param0) => _complete();
  method9(List<List<MojoHandle>> param0) {
    if (param0 != null) {
      param0.forEach((l) => l.forEach((h) {
            if (h != null) h.close();
          }));
    }
    _complete();
  }

  method10(Map<String, int> param0) => _complete();
  method11(StructG param0) => _complete();
  method12(double param0, [Function responseFactory]) {
    // If there are ever any passing method12 tests added, then this may need
    // to change.
    assert(responseFactory != null);
    _complete();
    return new Future.value(responseFactory(0.0));
  }

  method13(InterfaceAProxy param0, int param1, InterfaceAProxy param2) {
    if (param0 != null) param0.close(immediate: true);
    if (param2 != null) param2.close(immediate: true);
    _complete();
  }

  method14(UnionA param0) => _complete();
  method15(StructH param0) => _complete();

  Future close({bool immediate: false}) => _stub.close(immediate: immediate);
}

Future runTest(
    String name, parser.ValidationParseResult test, String expected) {
  var handles = new List.generate(
      test.numHandles, (_) => new MojoSharedBuffer.create(10).handle);
  var pipe = new MojoMessagePipe();
  var completer = new Completer();
  var conformanceImpl;

  conformanceImpl =
      new ConformanceTestInterfaceImpl(completer, pipe.endpoints[0]);
  conformanceImpl.onError = ((e) {
    assert(e.error is MojoCodecError);
    // TODO(zra): Make the error messages conform?
    // assert(e == expected);
    conformanceImpl.close();
    pipe.endpoints[0].close();
    pipe.endpoints[1].close();
    handles.forEach((h) => h.close());
    completer.completeError(null);
  });

  var length = (test.data == null) ? 0 : test.data.lengthInBytes;
  int r = pipe.endpoints[1].write(test.data, length, handles);
  assert(r == MojoResult.kOk);

  return completer.future.then((_) {
    assert(expected == "PASS");
    return conformanceImpl.close().then((_) {
      pipe.endpoints[0].close();
      pipe.endpoints[1].close();
      handles.forEach((h) => h.close());
    });
  }, onError: (e) {
    // Do nothing.
  });
}

main(List args) async {
  List<String> testData = args;

  // First test the parser.
  parser.parserTests();

  // See CollectTests in validation_unittest.cc for information on testData
  // format.
  for (var i = 0; i < testData.length; i += 3) {
    var name = testData[i + 0];
    var data = testData[i + 1].trim();
    var expected = testData[i + 2].trim();
    try {
      await runTest(name, parser.parse(data), expected);
      print('$name PASSED.');
    } catch (e) {
      print('$name FAILED: $e');
    }
  }
  // TODO(zra): Add integration tests when they no longer rely on Client=
  MojoHandle.reportLeakedHandles();
}
