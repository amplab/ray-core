# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for the logic that drives runs of the benchmarking mojo app and parses
its output."""

import imp
import os.path
import sys
import unittest

try:
  imp.find_module("devtoolslib")
except ImportError:
  sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from devtoolslib import benchmark


class BenchmarkTest(unittest.TestCase):
  """Tests the benchmark module."""

  def test_parse_measurement_results_empty(self):
    """Tests parsing empty output."""
    output = """"""
    results = benchmark._parse_measurement_results(output)
    self.assertEquals({}, results)

  def test_parse_measurement_results_typical(self):
    """Tests parsing typical output with unrelated log entries."""
    output = """
[INFO:network_fetcher.cc(322)] Caching mojo app http://127.0.0.1:31839/benchmark.mojo at /usr/local/google/home/user/.mojo_url_response_disk_cache/cache/4F6FAE752C7958AE122C6A2D778F2014C15578250B3C6746D54B99E4F15A4458/4F6FAE752C7958AE122C6A2D778F2014C15578250B3C6746D54B99E4F15A4458
[INFO:network_fetcher.cc(322)] Caching mojo app http://127.0.0.1:31839/dart_traced_application.mojo at /usr/local/google/home/user/.mojo_url_response_disk_cache/cache/AB290478907A1DC5434CBCFD053BE2E74254D882644E76B3C28E3E7E1BCDCC3D/AB290478907A1DC5434CBCFD053BE2E74254D882644E76B3C28E3E7E1BCDCC3D
Observatory listening on http://127.0.0.1:38128
[1109/155613:WARNING:event.cc(234)] Ignoring incorrect complete event (no duration)
measurement: time_until/a/b 42.5
measurement: time_between/a/b/c/d 21.1
measurement: time_between/a/b/e/f FAILED
some measurements failed
"""
    results = benchmark._parse_measurement_results(output)
    self.assertEquals({'time_until/a/b': 42.5,
                       'time_between/a/b/c/d': 21.1}, results)
