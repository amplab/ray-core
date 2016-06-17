# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Python utils."""

import os
import sys


def overrides(parent_class):
  """Inherits the docstring from the method of the same name in the indicated
  parent class.
  """
  def overriding(method):
    assert(method.__name__ in dir(parent_class))
    method.__doc__ = getattr(parent_class, method.__name__).__doc__
    return method
  return overriding


def disable_output_buffering():
  """Disables the buffering of the stdout. Devtools command line scripts should
  do so, so that their stdout is consistent when not directly attached to a
  terminal (e.g. because another script runs devtools in a subprocess).
  """
  sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)
