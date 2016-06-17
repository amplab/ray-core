# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Error-handling support functions."""


import os.path
import sys


def FatalError(message):
  print >> sys.stderr, "%s: fatal error: %s" % (os.path.basename(sys.argv[0]),
                                                message)
  sys.exit(1)
