#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A tool that runs a perf test and uploads the resulting data to the
performance dashboard.

By default uploads to a local testing dashboard assumed to be running on the
host. To run such server, check out Catapult and follow instructions at
https://github.com/catapult-project/catapult/blob/master/dashboard/README.md .
"""

# TODO(yzshen): The following are missing currently:
#     (1) CL range on the dashboard;
#     (2) improvement direction on the dashboard;
#     (3) a link from the build step pointing to the dashboard page.

import argparse
import subprocess
import sys
import re

import devtools
devtools.add_lib_to_path()
from devtoolslib import perf_dashboard

_PERF_LINE_FORMAT = r"""^\s*([^\s/]+)  # chart name
                        (/([^\s/]+))?  # trace name (optional, separated with
                                       # the chart name by a '/')
                        \s+(\S+)       # value
                        \s+(\S+)       # units
                        \s*$"""

_PERF_LINE_REGEX = re.compile(_PERF_LINE_FORMAT, re.VERBOSE)


def _ConvertPerfDataToChartFormat(perf_data, test_name):
  """Converts the perf data produced by a perf test to the "chart_data" format
  accepted by the performance dashboard, see:
  http://www.chromium.org/developers/speed-infra/performance-dashboard/sending-data-to-the-performance-dashboard.

  Returns:
    A dictionary that (after being converted to JSON) conforms to the server
    format.
  """
  charts = {}
  for line in perf_data:
    match = re.match(_PERF_LINE_REGEX, line)
    assert match, "Unable to parse the following input: %s" % line

    chart_name = match.group(1)
    trace_name = match.group(3) if match.group(3) else "summary"

    if chart_name not in charts:
      charts[chart_name] = {}
    charts[chart_name][trace_name] = {
        "type": "scalar",
        "value": float(match.group(4)),
        "units": match.group(5)
    }

  return {
      "format_version": "1.0",
      "benchmark_name": test_name,
      "charts": charts
  }


def main():
  parser = argparse.ArgumentParser(
      description="A tool that runs a perf test and uploads the resulting data "
                  "to the performance dashboard.")

  perf_dashboard.add_argparse_server_arguments(parser)
  parser.add_argument(
      "--perf-data-path",
      help="The path to the perf data that the perf test generates.")
  parser.add_argument("command", nargs=argparse.REMAINDER)
  args = parser.parse_args()

  subprocess.check_call(args.command)

  if not args.upload:
    return 0

  if not args.test_name or not args.perf_data_path:
    print ("Can't upload perf data to the dashboard because not all of the "
           "following values are specified: test-name, perf-data-path.")
    return 1

  with open(args.perf_data_path, "r") as perf_data:
    chart_data = _ConvertPerfDataToChartFormat(perf_data, args.test_name)

  result = perf_dashboard.upload_chart_data(
      args.master_name, args.bot_name, args.test_name, args.builder_name,
      args.build_number, chart_data, args.server_url,
      args.dry_run)

  return 0 if result else 1


if __name__ == '__main__':
  sys.exit(main())
