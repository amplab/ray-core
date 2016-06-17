#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# See https://github.com/domokit/mojo/wiki/Release-process

"""Updates the mojo_sdk package to require the latest versions of all leaf
packages and updates the version number in and CHANGELOG."""

import argparse
import os
import subprocess
import yaml

from release_common import \
    run, get_pubspec_version, build_package_map, update_pubspec_dependency, \
    MOJO_SDK_SRC_DIR, MOJO_SDK_PUBSPEC, PUB, SRC_DIR

def main():
  parser = argparse.ArgumentParser(description='Rev Mojo Dart SDK package')
  parser.parse_args()

  # Make the mojo_sdk package depend on the current versions of all leaf
  # packages. The current versions are taken from the source tree and not
  # the pub server. After updating the required dependency versions, this
  # script will verify that the pub can satisfy the package constraints.
  # This means that someone must have published the packages to pub and
  # that pub and the source tree agree on the current version number of
  # each leaf package.
  print('Updating leaf package dependencies to latest...')
  leaf_packages = ['mojo',
                   'mojo_services']
  package_map = build_package_map(leaf_packages)
  for leaf_package in package_map:
    leaf_package_dir = package_map[leaf_package]
    assert(leaf_package_dir != None)
    leaf_package_pubspec = os.path.join(leaf_package_dir, 'pubspec.yaml')
    # Get current the version number for leaf_package.
    leaf_package_version = get_pubspec_version(leaf_package_pubspec)
    # Update the mojo_sdk pubspec to depend the current version number.
    update_pubspec_dependency(MOJO_SDK_PUBSPEC,
                              leaf_package,
                              leaf_package_version)

  # Verify that pub can find all required package versions.
  run(MOJO_SDK_SRC_DIR, [PUB, 'get', '-n'])

  # Now, rev package.
  run(SRC_DIR,
      ['mojo/dart/tools/release/rev_pub_package.py',
       '--packages',
       'mojo_sdk'])

if __name__ == '__main__':
    main()
