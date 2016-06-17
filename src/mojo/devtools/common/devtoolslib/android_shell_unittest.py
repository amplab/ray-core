# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for the Android shell abstraction."""

import imp
import os.path
import sys
import unittest

try:
  imp.find_module("devtoolslib")
except ImportError:
  sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from devtoolslib.android_shell import parse_adb_devices_output

class AndroidShellTest(unittest.TestCase):
  """Tests the Android shell abstraction."""

  def test_parse_adb_devices_output(self):
    """Tests parsing of the `adb devices` output."""
    one_device_output = """\
List of devices attached
42424242        device

"""
    results = parse_adb_devices_output(one_device_output)
    self.assertEquals(1, len(results))
    self.assertTrue('42424242' in results)
    self.assertEquals('device', results['42424242'])

    multi_devices_output = """\
List of devices attached
42424242        device
42424243        offline
42424244        device

"""
    results = parse_adb_devices_output(multi_devices_output)
    self.assertEquals(3, len(results))
    self.assertTrue('42424242' in results)
    self.assertEquals('device', results['42424242'])
    self.assertTrue('42424243' in results)
    self.assertEquals('offline', results['42424243'])
    self.assertTrue('42424244' in results)
    self.assertEquals('device', results['42424244'])

    # Output produced when adb server needs to be started.
    adb_server_startup_output = """\
* daemon not running. starting it now on port 5037 *
* daemon started successfully *
List of devices attached
42424242        device

"""
    results = parse_adb_devices_output(adb_server_startup_output)
    self.assertEquals(1, len(results))
    self.assertTrue('42424242' in results)
    self.assertEquals('device', results['42424242'])
