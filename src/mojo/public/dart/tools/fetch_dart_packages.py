#!/usr/bin/env python
#
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility for updating third_party packages used in the Mojo Dart SDK"""

import argparse
import errno
import json
import os
import shutil
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(sys.argv[0])

def check_for_pubspec_yaml(directory):
  return os.path.exists(os.path.join(directory, 'pubspec.yaml'))

def change_directory(directory):
  return os.chdir(directory)

def list_packages(directory):
  for child in os.listdir(directory):
    path = os.path.join(directory, child)
    if os.path.isdir(path):
      print(child)

def remove_existing_packages(base_path):
  print('Removing all package directories under %s' % base_path)
  for child in os.listdir(base_path):
    path = os.path.join(base_path, child)
    if os.path.isdir(path):
      print('Removing %s ' % path)
      shutil.rmtree(path)

def pub_get(pub_exe):
  return subprocess.check_output([pub_exe, 'get'])

def copy_packages(base_path):
  packages_path = os.path.join(base_path, 'packages')
  for package in os.listdir(packages_path):
    lib_path = os.path.realpath(os.path.join(packages_path, package))
    package_path = os.path.normpath(os.path.join(lib_path, '..'))
    destinaton_path = os.path.join(base_path, package)
    print('Copying %s to %s' % (package_path, destinaton_path))
    shutil.copytree(package_path, destinaton_path)

def cleanup(base_path):
  shutil.rmtree(os.path.join(base_path, 'packages'), True)
  shutil.rmtree(os.path.join(base_path, '.pub'), True)
  os.remove(os.path.join(base_path, '.packages'))

def main():
  parser = argparse.ArgumentParser(description='Update third_party packages')
  parser.add_argument(
      '--pub-exe',
      action='store',
      metavar='pub_exe',
      help='Path to the pub executable',
      default='../../../../third_party/dart-sdk/dart-sdk/bin/pub')
  parser.add_argument('--directory',
                      action='store',
                      metavar='directory',
                      help='Directory containing pubspec.yaml',
                      default='.')
  parser.add_argument('--list',
                      help='List mode',
                      action='store_true',
                      default=False)
  args = parser.parse_args()
  args.directory = os.path.abspath(args.directory)

  if not check_for_pubspec_yaml(args.directory):
    print('Error could not find pubspec.yaml')
    return 1

  if args.list:
    list_packages(args.directory)
  else:
    remove_existing_packages(args.directory)
    change_directory(args.directory)
    print('Running pub get')
    pub_get(args.pub_exe)
    copy_packages(args.directory)
    print('Cleaning up')
    cleanup(args.directory)

if __name__ == '__main__':
  sys.exit(main())
