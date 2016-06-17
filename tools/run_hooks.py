#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script runs all of the hooks specified in DEPS in order.

import os
import subprocess
import sys

scope = {}

def Var(key):
  return scope["vars"][key]

scope["Var"] = Var

def main(args):
  gclient_path = os.path.abspath(os.path.join(os.path.dirname(__file__),
    os.pardir, os.pardir))
  deps_path = os.path.join(gclient_path, "src", "DEPS")
  with open(deps_path) as deps_contents:
    d = deps_contents.read()
  exec(d, scope)
  env = os.environ
  # Some hooks expect depot_tools to be in the PATH already so add the default
  # location of depot_tools in a standard checkout to the environment the hooks
  # run in.
  default_depot_tools_path = os.path.abspath(os.path.join(gclient_path,
      "depot_tools"))
  env["PATH"] += os.pathsep + default_depot_tools_path
  for hook in scope["hooks"]:
    name = hook["name"]
    print "________ running '%s' in '%s'" % (" ".join(hook["action"]),
        gclient_path)
    subprocess.check_call(hook["action"], cwd=gclient_path, env=env)
    print
  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
