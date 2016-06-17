#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# See https://github.com/domokit/mojo/wiki/Release-process

"""Updates peers to the mojo_sdk package to require the current version of
the sdk and updates the version number in and CHANGELOG."""

import argparse
import os
import subprocess
import yaml

from release_common import \
    run, get_pubspec_version, build_package_map, update_pubspec_dependency, \
    MOJO_SDK_PUBSPEC, PUB, SRC_DIR

def main():
  parser = argparse.ArgumentParser(
      description='Rev Mojo Dart SDK peer packages')
  parser.parse_args()

  # Make the mojo_sdk peer packages depend on the current version of the
  # mojo_sdk package. The current versions are taken from the source tree and
  # not the pub server. After updating the required dependency versions, this
  # script will verify that the pub can satisfy the package constraints.
  # This means that someone must have published the packages to pub and
  # that pub and the source tree agree on the current version number of
  # each leaf package.
  print('Updating peer package dependencies to latest...')
  peer_packages = ['mojom',
                   'mojo_apptest']
  package_map = build_package_map(peer_packages)
  for peer_package in package_map:
    peer_package_dir = package_map[peer_package]
    assert(peer_package_dir != None)
    peer_package_pubspec = os.path.join(peer_package_dir, 'pubspec.yaml')
    # Get current the version number of the mojo_sdk.
    mojo_sdk_package_version = get_pubspec_version(MOJO_SDK_PUBSPEC)
    # Update the peer package's pubspec to depend on the current mojo_sdk
    # version number.
    update_pubspec_dependency(peer_package_pubspec,
                              'mojo_sdk',
                              mojo_sdk_package_version)
    # Verify that pub can find all required package versions.
    run(peer_package_dir, [PUB, 'get', '-n'])

    # Now, rev package version.
    run(SRC_DIR,
        ['mojo/dart/tools/release/rev_pub_package.py',
         '--packages',
         peer_package])

if __name__ == '__main__':
    main()
