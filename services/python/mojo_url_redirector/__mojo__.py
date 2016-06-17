# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Application that serves requests for Mojo apps and responds with redirects
to the locations of the most recent versions of these apps."""

import logging
import os
import urlparse

import http_request_mojom
import http_response_mojom
import http_server_mojom
import http_server_factory_mojom
import net_address_mojom
import network_service_mojom
import service_provider_mojom
import url_request_mojom
import url_loader_mojom

from mojo_application import application_delegate
from mojo_application import application_impl
from mojo_application import application_runner

from mojo_bindings import promise
import mojo_system
from mojo_utils import data_pipe_utils


def _HttpResponseWithStatusCode(status_code):
  response = http_response_mojom.HttpResponse()
  response.status_code = status_code
  return response


def _HttpBadRequestResponse():
  return _HttpResponseWithStatusCode(400)


def _HttpNotFoundResponse():
  return _HttpResponseWithStatusCode(404)


def _HttpInternalServerErrorResponse():
  return _HttpResponseWithStatusCode(500)


def _HttpRedirectResponse(new_location):
  response = _HttpResponseWithStatusCode(302)
  custom_headers = {}
  custom_headers["location"] = new_location
  response.custom_headers = custom_headers
  return response


def _RequestIdentifier(requested_app, requested_platform):
  return "[Request: %s on %s]" % (requested_app, requested_platform)


def _LogMessageForRequest(request_identifier, message, log=logging.info,
                          exc_info=None):
  log("%s %s" % (request_identifier, message), exc_info=exc_info)


class MojoUrlRedirector(http_server_mojom.HttpHandler):
  def __init__(self, network_service, app_location_files_url):
    self.network_service = network_service
    self.app_location_files_url = app_location_files_url

  def HandleRequest(self, request):
    # Parse the components of the request.
    relative_url_components = request.relative_url.split("/")

    # The request must have a component for the platform and for the app.
    if len(relative_url_components) != 3:
      return _HttpBadRequestResponse()

    requested_platform = relative_url_components[1]
    requested_app = relative_url_components[2]

    def OnRedirectToAppRejected(error):
      request_identifier = _RequestIdentifier(requested_app, requested_platform)
      _LogMessageForRequest(request_identifier,
                            "Error in redirecting to current app location",
                            log=logging.error,
                            exc_info=getattr(error, "__traceback__", True))
      return _HttpInternalServerErrorResponse()

    return self.RedirectToCurrentAppLocation(requested_platform,
                                             requested_app).Catch(
        OnRedirectToAppRejected)

  def RedirectToCurrentAppLocation(self, requested_platform, requested_app):
    # Construct a URLRequest to fetch the app location file...
    app_location_request = url_request_mojom.UrlRequest()
    app_location_request.url = "%s/%s/%s_location" % (
        self.app_location_files_url, requested_platform, requested_app)
    app_location_request.auto_follow_redirects = True

    # ...and start a URLLoader to fetch it.
    url_loader_proxy, request = url_loader_mojom.UrlLoader.manager.NewRequest()
    self.network_service.CreateUrlLoader(request)

    return self.ProcessAppLocationResponse(
        url_loader_proxy.Start(app_location_request),
        _RequestIdentifier(requested_app, requested_platform),
        url_loader_proxy)

  @promise.async
  def ProcessAppLocationResponse(self, app_location_response,
                                 request_identifier, url_loader_proxy):
    error_message = None
    if app_location_response.error:
      error_message = "Network error from app location fetch: %d" % (
          app_location_response.error.code)
    elif app_location_response.status_code != 200:
      error_message = "Unexpected http status from app location fetch: %s" % (
          app_location_response.status_code)

    if error_message:
      _LogMessageForRequest(request_identifier, error_message,
                            log=logging.error)
      return _HttpNotFoundResponse()

    return self.ProcessAppLocationResponseBody(
        data_pipe_utils.CopyFromDataPipe(app_location_response.body,
                                         30 * 10**6),
        request_identifier)

  @promise.async
  def ProcessAppLocationResponseBody(self, app_location_body,
                                     request_identifier):
    app_location = app_location_body.decode("utf-8")
    if not urlparse.urlparse(app_location).scheme:
      # This is a Google storage path.
      app_location = "https://storage.googleapis.com/" + app_location
    _LogMessageForRequest(request_identifier,
                          "Redirecting to %s" % app_location)
    return _HttpRedirectResponse(app_location)


class MojoUrlRedirectorApp(application_delegate.ApplicationDelegate):
  def Initialize(self, shell, application):
    server_address = net_address_mojom.NetAddress()
    server_address.family = net_address_mojom.NetAddressFamily.IPV4
    server_address.ipv4 = net_address_mojom.NetAddressIPv4()
    server_address.ipv4.addr = [0, 0, 0, 0]
    server_address.ipv4.port = 80
    app_location_files_url = "https://storage.googleapis.com/mojo/services"

    # Parse args if given.
    if application.args:
      assert len(application.args) >= 2
      server_address_arg = application.args[1]
      server_address_str, server_port_str = server_address_arg.split(":")
      server_address.ipv4.addr = [int(n) for n in server_address_str.split(".")]
      server_address.ipv4.port = int(server_port_str)

      if len(application.args) == 3:
        app_location_files_url = application.args[2]

    # Connect to HttpServer.
    http_server_factory = application.ConnectToService("mojo:http_server",
        http_server_factory_mojom.HttpServerFactory)
    self.http_server, request = (
        http_server_mojom.HttpServer.manager.NewRequest())
    http_server_factory.CreateHttpServer(request, server_address)

    # Connect to NetworkService.
    self.network_service = application.ConnectToService("mojo:network_service",
        network_service_mojom.NetworkService)

    # Construct a MojoUrlRedirector and add that as a handler to the server.
    self.app_request_handler = MojoUrlRedirector(self.network_service,
                                                 app_location_files_url)
    self.http_server.SetHandler("/.*", self.app_request_handler)


def MojoMain(app_request_handle):
  application_runner.RunMojoApplication(MojoUrlRedirectorApp(),
                                        app_request_handle)
