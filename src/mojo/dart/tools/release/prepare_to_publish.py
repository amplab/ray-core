#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# See https://github.com/domokit/mojo/wiki/Release-process

"""This tool ensures that Dart pub packages can be released by building
the android Release configuration and running the bindings checker"""

import argparse
import distutils.util
import os
import subprocess
import sys
import tempfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.dirname(
    os.path.dirname(
        os.path.dirname(
            os.path.dirname(SCRIPT_DIR))))

OUT_DIR = os.path.join(SRC_DIR, 'out', 'android_Release')


def run(cwd, args):
    print 'RUNNING:', ' '.join(args), 'IN:', cwd
    subprocess.check_call(args, cwd=cwd)


CONFIRM = """
This tool is destructive and will revert your current branch to origin/master
and requires building Android release. Are you sure you wish to continue?
"""

def confirm(prompt):
    user_input = raw_input("%s (y/N) " % prompt)
    try:
        return distutils.util.strtobool(user_input) == 1
    except ValueError:
        return False


def main():
    if not confirm(CONFIRM):
        print "Aborted."
        return 1

    run(SRC_DIR, ['git', 'fetch', 'origin'])
    run(SRC_DIR, ['git', 'reset', 'origin/master', '--hard'])
    run(SRC_DIR, ['gclient', 'sync'])

    run(SRC_DIR, ['mojo/tools/mojob.py', 'gn', '--android', '--release'])
    run(SRC_DIR, ['mojo/tools/mojob.py', 'build', '--android', '--release'])

    run(SRC_DIR, ['mojo/dart/tools/presubmit/check_mojom_dart.py'])

    print("""
Pre-release checks successful.

To release packages:
$ cd mojo/dart/packages/<package>
$ pub lish
""")


if __name__ == '__main__':
    sys.exit(main())
