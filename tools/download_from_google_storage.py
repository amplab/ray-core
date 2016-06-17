#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# depot_tools' download_from_google_storage.py expects to be run from the parent
# of the 'src' directory so that paths like 'src/buildtools/linux64/gn.sha1'
# resolve. This script exists at src/tools/ and sets the working directory for
# download_from_google_storage.py so that it can be run from anywhere.

import os
import sys
import subprocess

def main(args):
  dir = os.path.abspath(os.path.join(os.path.dirname(__file__),
                        os.pardir, os.pardir))
  subprocess.check_call(['download_from_google_storage.py'] + args, cwd=dir)

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
