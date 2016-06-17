# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess
import os
import yaml

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.dirname(
    os.path.dirname(
        os.path.dirname(
            os.path.dirname(SCRIPT_DIR))))

PACKAGES_DIR = os.path.join(SRC_DIR, 'mojo', 'dart', 'packages')

DART_SDK = os.path.join(SRC_DIR, 'third_party', 'dart-sdk', 'dart-sdk', 'bin')
PUB = os.path.join(DART_SDK, 'pub')

MOJO_SDK_SRC_DIR = os.path.join(SRC_DIR, 'mojo/dart/packages/mojo_sdk/')
MOJO_SDK_PUBSPEC = os.path.join(MOJO_SDK_SRC_DIR, 'pubspec.yaml')

def run(cwd, args):
  print 'RUNNING:', ' '.join(args), 'IN:', cwd
  subprocess.check_call(args, cwd=cwd)

# Given the path to a pubspec.yaml file, return the version.
def get_pubspec_version(pubspec):
  with open(pubspec, 'r') as stream:
    spec = yaml.load(stream)
    return spec['version']

# Builds a map of package name to package source directory.
def build_package_map(package_list):
  packages = {}
  for package in os.listdir(PACKAGES_DIR):
    # Skip private packages.
    if package.startswith('_'):
      continue
    # Skip packages we don't care about.
    if not (package in package_list):
      continue
    package_path = os.path.join(PACKAGES_DIR, package)
    # Skip everything but directories.
    if not os.path.isdir(package_path):
      continue
    packages[package] = package_path
  return packages

# Update pubspec so that it depends on package: new_version.
def update_pubspec_dependency(pubspec, package, new_version):
  # TODO(johnmccutchan): Call out to psye tool to update version number.
  with open(pubspec, 'r') as stream:
    spec = yaml.load(stream)
    dependencies = spec['dependencies']
    assert(dependencies != None)
    # Extract the version we currently depend on.
    version = dependencies.get(package)
    # Handle the case where package is new or missing from the pubspec.
    if version == None:
      version = ''
    assert(version != None)
    # Update the version to the latest.
    dependencies[package] = new_version
    if version == new_version:
      print "%20s no update for %20s  %6s" % (spec['name'],
                                              package,
                                              version)
      return
    print "%20s %20s  %6s => %6s" % (spec['name'],
                                     package,
                                     version,
                                     dependencies[package])
  with open(pubspec, 'w') as stream:
    yaml.dump(spec, stream=stream, default_flow_style=False)
