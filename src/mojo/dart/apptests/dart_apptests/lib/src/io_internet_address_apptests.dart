// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library io_internet_address_apptests;

import 'dart:async';
import 'dart:io';

import 'package:mojo_apptest/apptest.dart';
import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

tests(Application application, String url) {
  group('InternetAddress Apptests', () {
    test('Parse IPv4', () async {
      var localhost = new InternetAddress('127.0.0.1');
      expect(localhost, equals(InternetAddress.LOOPBACK_IP_V4));
    });
    test('Parse IPv6', () async {
      var localhost = new InternetAddress('0:0:0:0:0:0:0:1');
      expect(localhost, equals(InternetAddress.LOOPBACK_IP_V6));
    });
    test('Reverse', () async {
      var result = await InternetAddress.LOOPBACK_IP_V4.reverse();
      expect(result.host, equals('localhost'));
    });
    test('Lookup IPv4', () async {
      var result =
          await InternetAddress.lookup('localhost',
                                       type:InternetAddressType.IP_V4);
      expect(result.length, greaterThan(0));
      expect(result[0], equals(InternetAddress.LOOPBACK_IP_V4));
    });
    test('Lookup IPv6', () async {
      try {
        var result =
            await InternetAddress.lookup('localhost',
                                         type:InternetAddressType.IP_V6);
        expect(result.length, greaterThan(0));
        expect(result[0], equals(InternetAddress.LOOPBACK_IP_V6));
      } on OSError catch (e) {
        expect(e.message, stringContainsInOrder(["ERR_NAME_NOT_RESOLVED"]));
      }
    });
    test('Lookup ANY', () async {
      var result =
          await InternetAddress.lookup('localhost',
                                       type:InternetAddressType.ANY);
      expect(result.length, greaterThan(0));
    });
    test('Lookup failure', () async {
      var result;
      var exceptionCaught = false;
      try {
        result = await InternetAddress.lookup('local.7778888');
        // Not hit.
        expect(true, isFalse);
      } on OSError catch(e) {
        exceptionCaught = true;
        expect(e.errorCode, isNot(equals(0)));
      }
      expect(exceptionCaught, isTrue);
    });
  });
}
