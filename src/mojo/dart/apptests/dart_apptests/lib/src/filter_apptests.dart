// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library filter_apptests;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:mojo_apptest/apptest.dart';
import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

var generateListTypes = [
  (list) => list,
  (list) => new Uint8List.fromList(list),
  (list) => new Int8List.fromList(list),
  (list) => new Uint16List.fromList(list),
  (list) => new Int16List.fromList(list),
  (list) => new Uint32List.fromList(list),
  (list) => new Int32List.fromList(list),
];

var generateViewTypes = [
  (list) => new Uint8List.view((new Uint8List.fromList(list)).buffer, 1, 8),
  (list) => new Int8List.view((new Int8List.fromList(list)).buffer, 1, 8),
  (list) => new Uint16List.view((new Uint16List.fromList(list)).buffer, 2, 6),
  (list) => new Int16List.view((new Int16List.fromList(list)).buffer, 2, 6),
  (list) => new Uint32List.view((new Uint32List.fromList(list)).buffer, 4, 4),
  (list) => new Int32List.view((new Int32List.fromList(list)).buffer, 4, 4),
];

void testZLibInflateSync(List<int> data) {
  [true, false].forEach((gzip) {
    [3, 6, 9].forEach((level) {
      var encoded = new ZLibEncoder(gzip: gzip, level: level).convert(data);
      var decoded = new ZLibDecoder().convert(encoded);
      expect(decoded, equals(data));
    });
  });
}

tests(Application application, String url) {
  group('Filter Apptests', () {
    test('ZLibDeflateEmpty', () async {
      Completer completer = new Completer();
      var controller = new StreamController(sync: true);
      controller.stream.transform(new ZLibEncoder(gzip: false, level: 6))
          .fold([], (buffer, data) {
            buffer.addAll(data);
            return buffer;
          })
          .then((data) {
            expect(data, equals([120, 156, 3, 0, 0, 0, 0, 1]));
            completer.complete(null);
          });
      controller.close();
      await completer.future;
    });
    test('ZLibDeflateEmptyGzip', () async {
      Completer completer = new Completer();
      var controller = new StreamController(sync: true);
      controller.stream.transform(new ZLibEncoder(gzip: true, level: 6))
          .fold([], (buffer, data) {
            buffer.addAll(data);
            return buffer;
          })
          .then((data) {
            expect(data.length > 0, isTrue);
            expect([], equals(new ZLibDecoder().convert(data)));
            completer.complete(null);
          });
      controller.close();
      await completer.future;
    });
    test('ZlibInflateThrowsWithSmallerWindow', () async {
      var data = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
      var encoder = new ZLibEncoder(windowBits: 10);
      var encodedData = encoder.convert(data);
      var decoder = new ZLibDecoder(windowBits: 8);
      bool caughtException = false;
      try {
        decoder.convert(encodedData);
        expect(true, isFalse);
      } catch (e) {
        caughtException = true;
      }
      expect(caughtException, isTrue);
    });
    test('ZlibInflateWithLargerWindow', () async {
      var data = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];

      Completer completer = new Completer();

      int doneCount = 0;
      const totalCount = 6;  // [true, false] * [3, 6, 9].

      [true, false].forEach((gzip) {
        [3, 6, 9].forEach((level) {
          var controller = new StreamController(sync: true);
          controller.stream
              .transform(new ZLibEncoder(gzip: gzip, level: level,
                                         windowBits: 8))
              .transform(new ZLibDecoder(windowBits: 10))
              .fold([], (buffer, data) {
                buffer.addAll(data);
                return buffer;
              })
              .then((inflated) {
                expect(inflated, equals(data));
                doneCount++;
                if (doneCount == totalCount) {
                  completer.complete(null);
                }
              });
          controller.add(data);
          controller.close();
        });
      });
      await completer.future;
    });
    test('ZlibWithDictionary', () async {
      var dict = [102, 111, 111, 98, 97, 114];
      var data = [98, 97, 114, 102, 111, 111];

      [3, 6, 9].forEach((level) {
        var encoded = new ZLibEncoder(level: level, dictionary: dict)
            .convert(data);
        var decoded = new ZLibDecoder(dictionary: dict).convert(encoded);
        expect(decoded, equals(data));
      });
    });
    test('List Types', () async {
      generateListTypes.forEach((f) {
        var data = f([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]);
        testZLibInflateSync(data);
      });
    });
    test('List View Types', () async {
      generateViewTypes.forEach((f) {
        var data = f([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]);
        testZLibInflateSync(data);
      });
    });
  });
}
