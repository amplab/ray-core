#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Builds and optionally uploads the mojom tool to Google Cloud Storage.

This script is used to build a new version of the mojom tool binary for both
mac and linux and optionally upload them to Google Cloud Storage based on their
SHA1 digest. It operates in 2 steps:
  1) Invokes the Go compiler to build the mojom parser for all
  target os and architectures.
  2) If the --upload flag is specified, for each of the binaries built:
    i) Computes the sha1 digest of the binary file.
    ii) Uploads the file to Google Cloud Storage using the sha1 as the filename.
    iii) Updates a local file named "mojom.sha1" to contain the sha1
    digest of the file that was uploaded. This allows "gclient sync" to download
    the file.

In order to use this script to upload a new version of the parser, you need:
- depot_tools in your path
- WRITE access to the bucket at
  https://console.developers.google.com/storage/browser/mojo/mojom_parser/.

If you want to hack on the parser and use your locally-built version, from the
Mojo source root directory do:
1) Run this script: mojom/mojom_parser/tools/build_mojom_tool.py
2) Copy the architecture-specific binary to the architecture-specific location:
For example for linux do:
cp mojom_linux_amd64  \
mojo/public/tools/bindings/mojom_tool/bin/linux64/mojom

This script will build for all architectures before it attempts to upload any
binary. If any of the builds fail, the whole script will abort and nothing will
be uploaded. This behavior can be overriden with the --keep-going flag.
"""

import os
import sys

import builder


def main():
  parser = builder.get_arg_parser("Build the mojom tool.")
  args = parser.parse_args()

  parser_builder = builder.get_builder(
      args=args,
      target_dir='',
      binary_name='mojom',
      src_path=os.path.join('mojom', 'mojom_tool'),
      friendly_name='mojom tool')

  if args.upload:
    return parser_builder.build_and_upload()
  return parser_builder.build()


if __name__ == '__main__':
  sys.exit(main())
