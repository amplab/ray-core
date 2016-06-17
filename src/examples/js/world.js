#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// See hello.js.
define("main", [
  "console",
  "mojo/services/public/js/application",
], function(console, application) {

  const Application = application.Application;

  class World extends Application {
    initialize(args) {
      console.log(this.url + ": World");
      this.quit();
    }
  }

  return World;
});
