#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs Dart example tests"""

# This script runs tests of example Dart Mojo apps.
# It looks for files named *_test.dart under subdirectories of
# //examples/dart in the EXAMPLES list, below. These tests are
# passed a command line argument --mojo-shell /path/to/mojo_shell that they
# can use to run the example.

import argparse
import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.dirname(os.path.dirname(SCRIPT_DIR))

DART_SDK = os.path.join(SRC_DIR, 'third_party', 'dart-sdk', 'dart-sdk', 'bin')
DART = os.path.join(DART_SDK, 'dart')

# Add subdirectories of //examples/dart here.
EXAMPLES = ['hello_world', 'wget']

def FindTests(example):
  tests = []
  for root, _, files in os.walk(os.path.join(SCRIPT_DIR, example)):
    for f in files:
      if f.endswith('_test.dart'):
        tests.append(os.path.join(root, f))
  return tests

def RunTests(tests, build_dir, package_root):
  for test in tests:
    test_path = os.path.join(SCRIPT_DIR, test)
    subprocess.check_call(
        [DART, '-p', package_root, '--checked', test_path,
         '--mojo-shell', os.path.join(build_dir, 'mojo_shell'),
         '--',
         '--args-for=mojo:dart_content_handler --disable-observatory'])

def main(build_dir, package_root):
  for example in EXAMPLES:
    tests = FindTests(example)
    RunTests(tests, build_dir, package_root)

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description="Run Dart example tests")
  parser.add_argument('-b', '--build-dir',
                      dest='build_dir',
                      metavar='<build-directory>',
                      type=str,
                      required=True,
                      help='The build output directory. E.g. out/Debug')
  args = parser.parse_args()
  package_root = os.path.join(
      SRC_DIR, args.build_dir, 'gen', 'dart-pkg', 'packages')
  sys.exit(main(args.build_dir, package_root))
