#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This script invokes the go build tool.
Must be called as follows:
python go.py [--android] <go-tool> <build directory> <output file>
<src directory> <CGO_CFLAGS> <CGO_LDFLAGS> <go-binary options>
eg.
python go.py /usr/lib/google-golang/bin/go out/build out/a.out .. "-I."
"-L. -ltest" test -c test/test.go
"""

import argparse
import os
import shutil
import sys

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--target_os', default='linux')
  parser.add_argument('go_tool')
  parser.add_argument('build_directory')
  parser.add_argument('output_file')
  parser.add_argument('src_root')
  parser.add_argument('out_root')
  parser.add_argument('cgo_cflags')
  parser.add_argument('cgo_ldflags')
  parser.add_argument('go_option', nargs='*')
  args = parser.parse_args()
  go_tool = os.path.abspath(args.go_tool)
  build_dir = args.build_directory
  out_file = os.path.abspath(args.output_file)
  go_options = args.go_option
  try:
    shutil.rmtree(build_dir, True)
    os.mkdir(build_dir)
  except Exception:
    pass

  sys.path.append(os.path.join(args.src_root, 'mojo', 'tools'))
  from mopy.invoke_go import InvokeGo

  call_result = InvokeGo(go_tool, go_options,
                         work_dir=build_dir,
                         src_root=args.src_root,
                         out_root=args.out_root,
                         cgo_cflags=args.cgo_cflags,
                         cgo_ldflags=args.cgo_ldflags,
                         target_os=args.target_os)
  if call_result != 0:
    return call_result
  file_list = [os.path.join(build_dir, f) for f in os.listdir(build_dir)]
  out_files = sorted([f for f in file_list if os.path.isfile(f)])
  if (len(out_files) > 0):
    shutil.move(out_files[0], out_file)
  try:
    shutil.rmtree(build_dir, True)
  except Exception:
    pass

if __name__ == '__main__':
  sys.exit(main())
