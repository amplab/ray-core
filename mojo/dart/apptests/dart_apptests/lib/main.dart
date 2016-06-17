// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'package:mojo_apptest/apptest.dart';

import 'src/connect_to_loader_apptests.dart' as connect_to_loader_apptests;
import 'src/echo_apptests.dart' as echo;
import 'src/file_apptests.dart' as file;
import 'src/filter_apptests.dart' as filter;
import 'src/io_http_apptests.dart' as io_http;
import 'src/io_internet_address_apptests.dart' as io_internet_address;
import 'src/pingpong_apptests.dart' as pingpong;
import 'src/service_describer_apptests.dart' as service_describer;
import 'src/uri_apptests.dart' as uri;
import 'src/versioning_apptests.dart' as versioning;

main(List args, Object handleToken) {
  final tests = [
    connect_to_loader_apptests.connectToLoaderApptests,
    echo.echoApptests,
    file.tests,
    filter.tests,
    io_internet_address.tests,
    io_http.tests,
    pingpong.pingpongApptests,
    service_describer.tests,
    uri.tests,
    versioning.tests
  ];
  runAppTests(handleToken, tests);
}
