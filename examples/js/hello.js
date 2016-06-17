#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Demonstrate one JS Mojo application connecting to another to emit "hello
// world". To run this application with mojo_shell, set DIR to be the absolute
// path for this directory, then:
//   mojo_shell "file://$DIR/hello.js file://$DIR/world.js"
// Launches the Mojo hello.js application which connects to the application
// URL specified as a Mojo application argument, world.js in this case.

define("main", [
  "console",
  "mojo/services/public/js/application",
], function(console, application) {

  const Application = application.Application;

  class Hello extends Application {
    initialize(args) {
      console.log(this.url + ": Hello");
      if (args && args.length == 2) // args is a nullable parameter
        this.shell.connectToApplication(args[1]);
      else
        console.log("Error: expected hello.js <URL for world.js>");
      this.quit();
    }
  }

  return Hello;
});
