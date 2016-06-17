// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library versioning_apptests;

import 'dart:async';

import 'package:mojo_apptest/apptest.dart';
import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:_mojo_for_test_only/mojo/test/versioning/versioning_test_client.mojom.dart';

tests(Application application, String url) {
  group('Versioning Apptests', () {
    test('Struct', () async {
      // The service side uses a newer version of Employee definition that
      // includes the 'birthday' field.

      // Connect to human resource database.
      var database = HumanResourceDatabase.connectToService(
          application, "mojo:versioning_test_service");

      // Query database and get a response back (even though the client does not
      // know about the birthday field).
      var c = new Completer();
      bool retrieveFingerPrint = true;
      database.queryEmployee(1, retrieveFingerPrint,
          (Employee e, List<int> fingerPrint) {
        c.complete([e, fingerPrint]);
      });
      var response = await database.responseOrError(c.future);
      expect(response[0].employeeId, equals(1));
      expect(response[0].name, equals("Homer Simpson"));
      expect(response[0].department, equals(Department.dev));
      expect(response[1], isNotNull);

      // Pass an Employee struct to the service side that lacks the 'birthday'
      // field.
      var newEmployee = new Employee();
      newEmployee.employeeId = 2;
      newEmployee.name = "Marge Simpson";
      newEmployee.department = Department.sales;
      c = new Completer();
      database.addEmployee(newEmployee, (bool success) {
        c.complete(success);
      });
      response = await database.responseOrError(c.future);
      expect(response, isTrue);

      // Query for employee #2.
      c = new Completer();
      retrieveFingerPrint = false;
      database.queryEmployee(2, retrieveFingerPrint,
          (Employee e, List<int> fingerPrint) {
        c.complete([e, fingerPrint]);
      });
      response = await database.responseOrError(c.future);
      expect(response[0].employeeId, equals(2));
      expect(response[0].name, equals("Marge Simpson"));
      expect(response[0].department, equals(Department.sales));
      expect(response[1], isNull);

      // Disconnect from database.
      database.close();
    });

    test('QueryVersion', () async {
      // Connect to human resource database.
      var database = HumanResourceDatabase.connectToService(
          application, "mojo:versioning_test_service");
      // Query the version.
      var version = await database.ctrl.queryVersion();
      // Expect it to be 1.
      expect(version, equals(1));
      // Disconnect from database.
      database.close();
    });

    test('RequireVersion', () async {
      // Connect to human resource database.
      var database = HumanResourceDatabase.connectToService(
          application, "mojo:versioning_test_service");

      // Require version 1.
      database.ctrl.requireVersion(1);
      expect(database.ctrl.version, equals(1));

      // Query for employee #3.
      var retrieveFingerPrint = false;
      var c = new Completer();
      database.queryEmployee(3, retrieveFingerPrint,
          (Employee e, List<int> fingerPrint) {
        c.complete([e, fingerPrint]);
      });
      var response = await database.responseOrError(c.future);

      // Got some kind of response.
      expect(response, isNotNull);

      // Require version 3 (which cannot be satisfied).
      database.ctrl.requireVersion(3);
      expect(database.ctrl.version, equals(3));

      // Query for employee #1, observe that the call fails.
      bool exceptionCaught = false;
      c = new Completer();
      database.queryEmployee(1, retrieveFingerPrint,
          (Employee e, List<int> fingerPrint) {
        c.complete([e, fingerPrint]);
      });
      try {
        response = await database.responseOrError(c.future);
        fail('Exception should be thrown.');
      } catch (e) {
        exceptionCaught = true;
      }
      expect(exceptionCaught, isTrue);

      // No need to disconnect from database because we were disconnected by
      // the requireVersion control message.
    });

    test('CallNonexistentMethod', () async {
      // Connect to human resource database.
      var database = HumanResourceDatabase.connectToService(
          application, "mojo:versioning_test_service");
      const fingerPrintLength = 128;
      var fingerPrint = new List(fingerPrintLength);
      for (var i = 0; i < fingerPrintLength; i++) {
        fingerPrint[i] = i + 13;
      }
      // Although the client side doesn't know whether the service side supports
      // version 1, calling a version 1 method succeeds as long as the service
      // side supports version 1.
      var c = new Completer();
      database.attachFingerPrint(1, fingerPrint, (bool success) {
        c.complete(success);
      });
      var response = await database.responseOrError(c.future);
      expect(response, isTrue);

      // Calling a version 2 method (which the service side doesn't support)
      // closes the pipe.
      bool exceptionCaught = false;
      c = new Completer();
      database.listEmployeeIds((List<int> ids) {
        c.complete(ids);
      });
      try {
        response = await database.responseOrError(c.future);
        fail('Exception should be thrown.');
      } catch (e) {
        exceptionCaught = true;
      }
      expect(exceptionCaught, isTrue);

      // No need to disconnect from database because we were disconnected by
      // the call to a version 2 method.
    });
  });
}
