#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Small utility script to simplify generating bindings"""

import argparse
import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.dirname(
    os.path.dirname(
        os.path.dirname(
            os.path.dirname(SCRIPT_DIR))))

MOJO_SDK = os.path.join(SRC_DIR, 'mojo', 'public')
DART_SDK = os.path.join(SRC_DIR, 'third_party', 'dart-sdk', 'dart-sdk', 'bin')
DART = os.path.join(DART_SDK, 'dart')
PUB = os.path.join(DART_SDK, 'pub')

PACKAGES_DIR = os.path.join(SRC_DIR, 'mojo', 'dart', 'packages')
MOJOM_PACKAGE_DIR = os.path.join(PACKAGES_DIR, 'mojom')
MOJOM_BIN = os.path.join(MOJOM_PACKAGE_DIR, 'bin', 'mojom.dart')

def run(cwd, args):
    print 'RUNNING:', ' '.join(args), 'IN:', cwd
    subprocess.check_call(args, cwd=cwd)


def main():
  parser = argparse.ArgumentParser(
      description='Generate source-tree Dart bindings')
  parser.add_argument('-f', '--force',
      default = False,
      help='Always generate all bindings.',
      action='store_true')
  parser.add_argument('-v', '--verbose',
      default = False,
      help='Verbose output.',
      action='store_true')
  args = parser.parse_args()

  extra_args = []
  if args.force:
    extra_args += ['-f']
  if args.verbose:
    extra_args += ['-v']

  run(MOJOM_PACKAGE_DIR, [PUB, 'get'])
  run(SRC_DIR, [DART,
                MOJOM_BIN,
                'gen',
                '-m',
                MOJO_SDK,
                '-r',
                SRC_DIR,
                '--output',
                PACKAGES_DIR] + extra_args)
  return 0

if __name__ == '__main__':
    sys.exit(main())
