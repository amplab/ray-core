# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for the native crash dump symbolizer utility functions."""

import unittest
import stack_utils


class StackUtilsTest(unittest.TestCase):
  """Tests the native crash dump symbolizer utility functions."""

  def test_GetSymbolMapping_no_match(self):
    """Verifies that, if no mapping is on the input, no mapping is on the
    output.
    """
    lines = ["This is a test case\n", "Caching all mojo apps", ""]
    self.assertDictEqual({}, stack_utils.GetSymbolMapping(lines))

  def test_GetSymbolMapping_simple_match(self):
    """Verifies a simple symbol mapping."""
    lines = ["This is a test case\n", "Caching all mojo apps",
        "I/mojo(2): [INFO:somefile.cc(85)] Caching mojo app "
        "https://apps.mojo/myapp.mojo at /path/to/myapp.mojo/.lM03ws"]
    golden_dict = {
        "/path/to/myapp.mojo/.lM03ws": "libmyapp_library.so"
    }
    actual_dict = stack_utils.GetSymbolMapping(lines)
    self.assertDictEqual(golden_dict, actual_dict)

  def test_GetSymbolMapping_multiple_match(self):
    """Verifies mapping of multiple symbol files."""
    lines = ["This is a test case\n", "Caching all mojo apps",
        "I/mojo(2): [INFO:somefile.cc(85)] Caching mojo app "
        "https://apps.mojo/myapp.mojo at /path/to/myapp.mojo/.lM03ws",
        "I/mojo(2): [INFO:somefile.cc(85)] Caching mojo app "
        "https://apps.mojo/otherapp.mojo at /path/to/otherapp.mojo/.kW07s"]
    golden_dict = {
        "/path/to/myapp.mojo/.lM03ws": "libmyapp_library.so",
        "/path/to/otherapp.mojo/.kW07s": "libotherapp_library.so"
    }
    actual_dict = stack_utils.GetSymbolMapping(lines)
    self.assertDictEqual(golden_dict, actual_dict)

  def test_GetSymbolMapping_parameter_match(self):
    """Verifies parameters are stripped from mappings."""
    lines = ["This is a test case\n", "Caching all mojo apps",
        "I/mojo(2): [INFO:somefile.cc(85)] Caching mojo app "
        "https://apps.mojo/myapp.mojo?q=hello at /path/to/myapp.mojo/.lM03ws"]
    golden_dict = {
        "/path/to/myapp.mojo/.lM03ws": "libmyapp_library.so"
    }
    actual_dict = stack_utils.GetSymbolMapping(lines)
    self.assertDictEqual(golden_dict, actual_dict)

  def test_GetSymbolMapping_normalize(self):
    """Verifies paths are normalized in mappings."""
    lines = ["This is a test case\n", "Caching all mojo apps",
        "I/mojo(2): [INFO:somefile.cc(85)] Caching mojo app "
        "https://apps.mojo/myapp.mojo at /path/to/.//myapp.mojo/.lM03ws"]
    golden_dict = {
        "/path/to/myapp.mojo/.lM03ws": "libmyapp_library.so"
    }
    actual_dict = stack_utils.GetSymbolMapping(lines)
    self.assertDictEqual(golden_dict, actual_dict)


if __name__ == "__main__":
  unittest.main()
