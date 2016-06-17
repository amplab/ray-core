#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Builds and optionally uploads the mojom generators to Google Cloud Storage.

This script works the same as build_mojom_tool.
"""

import os
import sys

import builder


def main():
  all_generators = set(['deps', 'go', 'c'])
  parser = builder.get_arg_parser('Build the mojom generators.')
  parser.add_argument('--generators', dest='generators', type=str,
      default=','.join(all_generators), action='store',
      help='Comma-separated list of generators to be built by this build '
      'script. These should be directories under mojom/generators. By default '
      'every generator known to the build script will be built.')
  args = parser.parse_args()

  generators = args.generators.split(',')
  for generator in generators:
    if generator not in all_generators:
      print "The only allowed generators are: %s" % all_generators.join(',')
      return -1

  final_result = 0
  for generator in generators:
    generator_builder = builder.get_builder(
        args=args,
        target_dir='generators',
        binary_name=generator,
        src_path=os.path.join('mojom', 'generators', generator))

    if args.upload:
      result = generator_builder.build_and_upload()
    result = generator_builder.build()
    if result != 0:
      final_result = result
      if not args.keep_going:
        return result

  return final_result


if __name__ == '__main__':
  sys.exit(main())
