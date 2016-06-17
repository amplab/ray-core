#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Demonstrate a Mojo wrapper for the Geocoder JSON API. The application
// connects to geocoder_service.js which implements geocoder.mojom.
// To run this application with mojo_shell, set DIR to be the absolute path
// for this directory, then:
//   mojo_shell file://$DIR/demo.js

define("main", [
  "console",
  "mojo/public/js/core",
  "mojo/public/js/unicode",
  "mojo/services/location/interfaces/geocoder.mojom",
  "mojo/services/location/interfaces/location.mojom",
  "mojo/services/public/js/application",
  "third_party/js/url",
], function(console, core, unicode, geocoder, location, application, url) {

  const Application = application.Application;
  const Geocoder = geocoder.Geocoder;
  const Result = geocoder.Result;
  const Location = location.Location;
  const Status = geocoder.Status;
  const Options = geocoder.Options;
  const URL = url.URL;

  var geocoderService;

  function demoAddressToLocation() {
    console.log("Demo GeocoderServce.AddressToLocation()");
    var addr = "1365 Shorebird way, Mountain View, CA";
    geocoderService.addressToLocation(addr, new Options).then(
      function(rv) {
        if (rv.status == Status.OK) {
          for (var i = 0; i < rv.results.length; i++) {
            var address = rv.results[i].formatted_address;
            var location = rv.results[i].geometry.location;
            console.log("Latitude,longitude for \"" + address + "\":");
            console.log(location.latitude + ", " + location.longitude);

            console.log("");
            demoLocationToAddress();
          }
        } else {
          console.log("Geocoder request failed status=" + rv.status);
        }
      });
  }

  function demoLocationToAddress() {
    console.log("Demo GeocoderServce.LocationToAddress()");
    var coords = new Location({
      latitude: 37.41811752319336,
      longitude: -122.07335662841797,
    });
    geocoderService.locationToAddress(coords, new Options).then(
      function(rv) {
        if (rv.status == Status.OK) {
          for (var i = 0; i < rv.results.length; i++) {
            var address = rv.results[i].formatted_address;
            var location = rv.results[i].geometry.location;
            console.log("Latitude,longitude for \"" + address + "\":");
            console.log(location.latitude + ", " + location.longitude);
          }
        } else {
          console.log("Geocoder request failed status=" + rv.status);
        }
      });
  }

  class Demo extends Application {
    initialize() {
      // TODO(alhaad): See if there is a better way to do this.
      var geocoderURL = new URL(this.url).resolve(
          "../../services/location/geocoder_service.js");
      geocoderService = this.shell.connectToService(geocoderURL, Geocoder);
      demoAddressToLocation();
    }
  }

  return Demo;
});

