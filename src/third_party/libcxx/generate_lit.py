# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script is used to generate configuration files for the lit
llvm testing tool.

The first argument to this script is the absolute path to a template
file. This file contains a number of variables for substitution,
surrounded by at signs. For example, in:

    config.cxx_under_test           = "@LIBCXX_COMPILER@"
    config.project_obj_root         = "@CMAKE_BINARY_DIR@"
    config.libcxx_src_root          = "@LIBCXX_SOURCE_DIR@"

LIBCXX_COMPILER, CMAKE_BINARY_DIR, and LIBCXX_SOURCE_DIR are variables
to be substituted.

The second argument to this script is the absolute path at which to
write out the result of the substitutions.

The remained of the arguments are pairs of variables and values:

    LIBCXX_COMPILER=clang CMAKE_BINARY_DIR=some/directory ...

which will be substituted.

"""

import sys


def parse_args(args):
  """Return the paths to the template and output files, and a list of
     key/value pairs to substitute.

  """

  template_path = args[0]
  output_path = args[1]

  substitutions = []

  for arg in args[2:]:
    arg_parts = arg.split('=', 1)
    assert(len(arg_parts) == 2)

    variable = '@' + arg_parts[0] + '@'
    value = arg_parts[1]

    substitutions.append((variable, value))

  return template_path, output_path, substitutions


def substitute(data, substitutions):
  """Perform all the substitutions into data, and return the new string.

  These files are hundreds of bytes, and there are maybe 20 things to
  substitute. So don't be clever, just loop and loop.

  """

  for variable, value in substitutions:
    data = data.replace(variable, value)

  return data


def main():
  template_path, output_path, substitutions = parse_args(sys.argv[1:])

  with open(template_path, 'r') as template_file:
    template_data = template_file.read()

  data = substitute(template_data, substitutions)

  with open(output_path, 'w') as output_file:
    output_file.write(data)


if __name__ == '__main__':
  main()
