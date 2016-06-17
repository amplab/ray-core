#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Demonstrate using the mojo window_manager application to "embed" a view that
// displays an image. To run this application set BUILD_DIR to the build
// directory (like "src/out/Debug") and append a PNG image URL as the url
// paramaeter for: absolute path for this directory, then:
//   sky/tools/mojodb start $BUILD_DIR examples/js/show_image.js?url=<PNG URL>
// The mojodb application starts an HTTP server that points at the build and
// and source directories. It starts a simple - just one view - window manager
// and then embeds this application in its root view. This application just
// asks the same window manager to embed the PNG viewer. Doing so effectively
// removes this application from the window manager's root view.

define("main", [
  "mojo/services/public/js/application",
  "mojo/services/public/js/service_provider",
  "mojo/services/window_manager/interfaces/window_manager.mojom",
  "third_party/js/url",
], function(application, serviceProvider, windowManager, url) {

  const Application = application.Application;
  const ServiceProvider = serviceProvider.ServiceProvider;
  const WindowManager = windowManager.WindowManager;
  const URL = url.URL;
  const defaultImageURL =
      "http://upload.wikimedia.org/wikipedia/commons/8/87/Google_Chrome_icon_%282011%29.png";

  class ShowImage extends Application {
    initialize() {
      var imageURL = new URL(this.url, true).query.url || defaultImageURL;
      var windowManager = this.shell.connectToService(
          "mojo:window_manager", WindowManager);
      windowManager.embed(imageURL, function() { /* no ServiceProvider */ });

      // Displaying imageURL is now the responsibility of the Mojo application
      // launched by its content handler. We're done.
      this.quit();
    }
  }

  return ShowImage;
});

