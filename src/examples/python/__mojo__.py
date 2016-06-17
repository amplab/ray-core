# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Example python application implementing the Echo service."""

import logging

import example_service_mojom
from mojo_application import application_delegate
from mojo_application import service_provider_impl
from mojo_application import application_runner

import mojo_system

class ExampleApp(application_delegate.ApplicationDelegate):
  def OnAcceptConnection(self,
                         requestor_url,
                         resolved_url,
                         service_provider):
    service_provider.AddService(ExampleServiceImpl)
    return True


class ExampleServiceImpl(example_service_mojom.ExampleService):
  def Ping(self, ping_value):
    return ping_value


def MojoMain(app_request_handle):
  application_runner.RunMojoApplication(ExampleApp(), app_request_handle)
