#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script runs the Dart content handler http load test in the mojo tree."""

import argparse
import os
import subprocess
import sys

MOJO_SHELL = 'mojo_shell'


def main(build_dir, dart_exe, tester_script, tester_dir):
  shell_exe = os.path.join(build_dir, MOJO_SHELL)
  subprocess.check_call([
    dart_exe,
    tester_script,
    shell_exe,
    tester_dir,
  ])

if __name__ == '__main__':
  parser = argparse.ArgumentParser(
      description="List filenames of files in the packages/ subdir of the "
                  "given directory.")
  parser.add_argument("--build-dir",
                      dest="build_dir",
                      metavar="<build-directory>",
                      type=str,
                      required=True,
                      help="The directory containing mojo_shell.")
  parser.add_argument("--dart-exe",
                      dest="dart_exe",
                      metavar="<package-name>",
                      type=str,
                      required=True,
                      help="Path to dart executable.")
  args = parser.parse_args()
  tester_directory = os.path.dirname(os.path.realpath(__file__))
  tester_dart_script = os.path.join(tester_directory, 'bin', 'tester.dart')
  package_directory = os.path.join(
      args.build_dir, 'gen', 'dart-pkg', 'mojo_dart_http_load_test')
  sys.exit(main(args.build_dir, args.dart_exe, tester_dart_script,
                package_directory))
