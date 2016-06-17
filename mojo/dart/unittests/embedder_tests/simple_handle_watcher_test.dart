// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:isolate';

import 'package:mojo/core.dart' as core;

void shouldSucceed() {
  var pipe = new core.MojoMessagePipe();
  assert(pipe != null);

  var endpoint = pipe.endpoints[0];
  assert(endpoint.handle.isValid);

  var eventSubscription = new core.MojoEventSubscription(endpoint.handle);
  var completer = new Completer();
  int numEvents = 0;

  eventSubscription.subscribe((_) {
    numEvents++;
    eventSubscription.close().then((_) {
      completer.complete(numEvents);
    });
  });
  eventSubscription.enableWriteEvents();

  completer.future.then((int numEvents) {
    assert(numEvents == 1);
  });
}

void doubleAdd() {
  var pipe = new core.MojoMessagePipe();
  assert(pipe != null);

  var endpoint = pipe.endpoints[0];
  assert(endpoint.handle.isValid);

  var eventSubscription1 = new core.MojoEventSubscription(endpoint.handle);
  var completer = new Completer();
  int numEvents = 0;

  eventSubscription1.subscribe((_) {
    numEvents++;
    eventSubscription1.close().then((_) {
      completer.complete(numEvents);
    });
  });

  // Create a second subscription using the same message pipe endpoint.
  var eventSubscription2 = new core.MojoEventSubscription(endpoint.handle);

  // Binding the message pipe endpoint to a second subscription should fail by
  // triggering the peer closed signal.
  eventSubscription2.subscribe((signal) {
    assert(core.MojoHandleSignals.isPeerClosed(signal));
    eventSubscription2.close();
  });

  // We should continue normally on the first binding.
  eventSubscription1.enableWriteEvents();

  completer.future.then((int numEvents) {
    assert(numEvents == 1);
  });
}

main() {
  shouldSucceed();
  doubleAdd();
}
