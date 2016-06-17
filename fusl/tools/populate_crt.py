# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os.path
import shutil
import subprocess
import sys


"""Populate the compiler-provided portions of the fusl sysroot.

In particular, crtbegin, ocrtend.o,libgcc.a, and so on must be copied
from the host sysroot to our sysroot.
"""

def parse_clang_dirs(clang):
  """Parse the output of |clang -print-search-dirs|.

  This is used to find library search paths. One line of the output
  is formatted something like:

  libraries: =/usr/lib/gcc/4.8:/lib:/usr/lib

  This functions strips the 'libraries: =' prefix and splits on the
  colon, returning the list:

  [ '/usr/lib/gcc/4.8', '/lib', '/usr/lib' ]
  """

  clang_search_dirs = subprocess.check_output([clang, '-print-search-dirs'])

  library_line = None
  library_line_prefix = 'libraries: ='
  for line in clang_search_dirs.split('\n'):
    if line.startswith(library_line_prefix):
      prefix_length = len(library_line_prefix)
      library_line = line[prefix_length:]
      break
  assert(library_line)
  return library_line.split(':')


def try_populate(artifact, library_paths, target):
  """Returns whether we found the artifact and copied it."""

  for library_path in library_paths:
    source = os.path.join(library_path, artifact)
    if not os.path.isfile(source):
      continue
    shutil.copy(source, target)
    return True

  return False


def main():
  clang = sys.argv[1]
  target = sys.argv[2]
  artifacts = sys.argv[3:]
  assert(len(artifacts))

  library_paths = parse_clang_dirs(clang)

  for artifact in artifacts:
    if not try_populate(artifact, library_paths, target):
      print('Unable to locate %s in any of the following paths: %s' %
            (artifact, library_paths))
      exit(1)


if __name__ == '__main__':
  main()
