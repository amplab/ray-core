# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import sys


def copytree(source_dir, target_dir):
  """Copy a tree.

  This is needed because shutil.copytree requires that |target| not
  exist yet.

  """

  for file in os.listdir(source_dir):
    source = os.path.join(source_dir, file)
    target = os.path.join(target_dir, file)
    if os.path.isfile(source):
      shutil.copy(source, target)
    else:
      copytree(source, target)


def main():
  source_path = sys.argv[1]
  target_path = sys.argv[2]
  copytree(source_path, target_path)


if __name__ == '__main__':
  main()
