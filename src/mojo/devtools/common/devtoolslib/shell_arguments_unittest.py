# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import imp
import os.path
import sys
import unittest

try:
  imp.find_module("devtoolslib")
except ImportError:
  sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from devtoolslib import shell_arguments


class AppendToArgumentTest(unittest.TestCase):
  """Tests AppendToArgument()."""

  def test_append_to_empty(self):
    arguments = []
    key = '--something='
    value = 'val'
    expected_result = ['--something=val']
    self.assertEquals(expected_result, shell_arguments.append_to_argument(
        arguments, key, value))

  def test_append_to_non_empty(self):
    arguments = ['--other']
    key = '--something='
    value = 'val'
    expected_result = ['--other', '--something=val']
    self.assertEquals(expected_result, shell_arguments.append_to_argument(
        arguments, key, value))

  def test_append_to_existing(self):
    arguments = ['--something=old_val']
    key = '--something='
    value = 'val'
    expected_result = ['--something=old_val,val']
    self.assertEquals(expected_result, shell_arguments.append_to_argument(
        arguments, key, value))


if __name__ == "__main__":
  unittest.main()
