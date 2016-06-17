#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import os
import sys

from mopy.mojo_python_tests_runner import MojoPythonTestRunner

class PythonMojomTestRunner(MojoPythonTestRunner):
  def __init__(self, test_dir, test_module=None):
    super(PythonMojomTestRunner, self).__init__(test_dir)
    self._test_module = test_module

  def add_custom_commandline_options(self, parser):
    parser.add_argument('--build-dir', action='store',
                        help='path to the build output directory')

  def apply_customization(self, args):
    if args.build_dir:
      sys.path.append(os.path.join(args.build_dir, 'python'))

  def get_test_suite(self, loader, pylib_dir):
    if pylib_dir not in sys.path:
      sys.path.append(pylib_dir)
    suite = unittest.TestSuite()
    suite.addTests(loader.loadTestsFromName(self._test_module))
    return suite



def main():
  test_dir = os.path.join(
      'mojo', 'public', 'tools', 'bindings', 'pylib', 'mojom', 'generate')
  test_module = 'mojom_translator_unittest'
  runner = PythonMojomTestRunner(test_dir, test_module=test_module)
  sys.exit(runner.run())

if __name__ == '__main__':
  sys.exit(main())
