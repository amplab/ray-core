// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:mojo/bindings.dart' as bindings;
import 'package:mojo/core.dart' as core;

main() {
  core.MojoMessagePipe pipe = new core.MojoMessagePipe();
  pipe.endpoints[0].handle.close();
  pipe.endpoints[1].handle.close();
}
