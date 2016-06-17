# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Apptest runner specific to the particular Dart apptest framework in
/mojo/dart/apptests, built on top of the general apptest runner."""

import logging
import re

_logging = logging.getLogger()

from devtoolslib.apptest import run_apptest

# The apptest framework guarantees that tearDownAll() is the last thing to be
# called before the shell is terminated. Note that the shell may be terminated
# before the "All tests passed!" string.
#
# So we pass on messages like:
#   00:01 +3: (tearDownAll)
# And fail on messages like:
#   00:01 +2 -1: (tearDownAll)
SUCCESS_PATTERN = re.compile(r'^\S+ \S+: \(tearDownAll\)', re.MULTILINE)


def _dart_apptest_output_test(output):
  return SUCCESS_PATTERN.search(output) is not None


# TODO(erg): Support android, launched services and fixture isolation.
def run_dart_apptest(shell, shell_args, apptest_url, apptest_args, timeout):
  """Runs a dart apptest.

  Args:
    shell_args: The arguments for mojo_shell.
    apptest_url: Url of the apptest app to run.
    apptest_args: Parameters to be passed to the apptest app.

  Returns:
    True iff the test succeeded, False otherwise.
  """
  return run_apptest(shell, shell_args, apptest_url, apptest_args, timeout,
                     _dart_apptest_output_test)
