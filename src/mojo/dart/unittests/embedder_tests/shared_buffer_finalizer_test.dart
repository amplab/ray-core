// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:isolate';
import 'dart:typed_data';
import 'dart:convert';

import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:_mojo_for_test_only/expect.dart';

void mapAndAccess(MojoSharedBuffer bufferHandle, int length) {
  ByteData mapping = bufferHandle.map(0, length, MojoSharedBuffer.mapFlagNone);
  if (bufferHandle.status != MojoResult.kOk) {
    throw "Bad shared buffer map: ${bufferHandle.status}";
  }

  Uint8List copy = new Uint8List(length);
  copy[0] = mapping.getUint8(0);

  bufferHandle.close();
}

int main() {
  MojoSharedBuffer bufferHandle;

  for (int i = 0; i < 1000; i++) {
    bufferHandle = new MojoSharedBuffer.create(30000);
    mapAndAccess(bufferHandle, 30000);
  }

  return 0;
}
