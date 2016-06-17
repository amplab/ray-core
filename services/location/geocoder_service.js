#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

define("main", [
  "console",
  "mojo/public/js/core",
  "mojo/public/js/unicode",
  "mojo/services/location/interfaces/geocoder.mojom",
  "mojo/services/location/interfaces/location.mojom",
  "mojo/services/public/js/application",
  "mojo/services/network/interfaces/network_service.mojom",
  "mojo/services/network/interfaces/url_loader.mojom",
  "third_party/js/url",
], function(console, core, unicode, geocoder, location, application, network,
  loader, url) {

  const Application = application.Application;
  const Bounds = geocoder.Bounds;
  const Geocoder = geocoder.Geocoder;
  const Geometry = geocoder.Geometry;
  const Location = location.Location;
  const NetworkService = network.NetworkService;
  const Result = geocoder.Result;
  const Status = geocoder.Status;
  const URLRequest = loader.URLRequest;
  const URL = url.URL;

  var netService;

  Location.prototype.queryString = function() {
    // TBD: format floats to 6 decimal places
    return this.latitude + ", " + this.longitude;
  }

  Location.fromJSON = function(json) {
    return !json ? null : new Location({
      latitude: json.lat,
      longitude: json.lng,
    });
  }

  Bounds.fromJSON = function(json) {
    return !json ? null : new Bounds({
      northeast: Location.fromJSON(json.northeast),
      southwest: Location.fromJSON(json.southwest),
    });
  }

  Geometry.fromJSON = function(json) {
    return !json ? null : new Geometry({
      location: Location.fromJSON(json.location),
      location_type: json.location_type,
      viewport: Bounds.fromJSON(json.viewport),
      bounds: Bounds.fromJSON(json.bounds),
    });
  }

  Result.fromJSON = function(json) {
    return !json ? null : new Result({
      partial_match: !!json.partial_match,
      formatted_address: json.formatted_address,
      geometry: Geometry.fromJSON(json.geometry),
      types: json.types,
      // TBD: address_components
    });
  }

  function parseGeocodeResponse(arrayBuffer) {
    return JSON.parse(unicode.decodeUtf8String(new Uint8Array(arrayBuffer)));
  }

  function geocodeRequest(url) {
    return new Promise(function(resolveRequest) {
      var urlLoader;
      netService.createURLLoader(function(urlLoaderProxy) {
        urlLoader = urlLoaderProxy;
      });

      var urlRequest = new URLRequest({
        url: url.format(),
        method: "GET",
        auto_follow_redirects: true
      });

      urlLoader.start(urlRequest).then(function(urlLoaderResult) {
        core.drainData(urlLoaderResult.response.body).then(
          function(drainDataResult) {
            // TBD: handle drainData failure
            var value = parseGeocodeResponse(drainDataResult.buffer);
            resolveRequest({
              status: value.status,
              results: value.results.map(Result.fromJSON),
            });
          });
      });
    });
  }

  function geocodeURL(key, value, options) {
    var url = new URL("https://maps.googleapis.com/maps/api/geocode/json");
    url.query = {};
    url.query[key] = value;
    // TBD: add options url.query
    return url;
  }

  class GeocoderImpl {
    addressToLocation(address, options) {
      return geocodeRequest(geocodeURL("address", address, options));
    }

    locationToAddress(location, options) {
      return geocodeRequest(
          geocodeURL("latlng", location.queryString(), options));
    }
  }

  class GeocoderService extends Application {
    initialize() {
      netService = this.shell.connectToService(
        "mojo:network_service", NetworkService);
    }

    acceptConnection(initiatorURL, serviceProvider) {
      serviceProvider.provideService(Geocoder, GeocoderImpl);
    }
  }

  return GeocoderService;
});

