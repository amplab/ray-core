#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs pure Go tests in the Mojo source tree from a list of directories
specified in a data file."""


import argparse
import os
import subprocess
import sys

from mopy.invoke_go import InvokeGo
from mopy.paths import Paths


def main():
  parser = argparse.ArgumentParser(description="Runs pure Go tests in the "
      "Mojo source tree from a list of directories specified in a data file.")
  parser.add_argument('go_tool_path', metavar='go-tool-path',
      help="the path to the 'go' binary")
  parser.add_argument("test_list_file", type=file, metavar='test-list-file',
                      help="a file listing directories containing go tests "
                      "to run")
  args = parser.parse_args()
  go_tool = os.path.abspath(args.go_tool_path)

  # Execute the Python script specified in args.test_list_file.
  # This will populate a list of Go directories.
  test_list_globals = {}
  exec args.test_list_file in test_list_globals
  test_dirs = test_list_globals["test_dirs"]

  src_root = Paths().src_root
  assert os.path.isabs(src_root)
  # |test_dirs| is a list of "/"-delimited, |src_root|-relative, paths
  # of directories containing a Go package.
  for path in test_dirs:
    test_dir = os.path.join(src_root, *path.split('/'))
    os.chdir(test_dir)
    print "Running Go tests in %s..." % test_dir
    call_result = InvokeGo(go_tool, ["test"], src_root=src_root)
    if call_result:
      return call_result

if __name__ == '__main__':
  sys.exit(main())
