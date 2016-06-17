# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Logic that drives runs of the benchmarking mojo app and parses its output."""

import os.path
import pipes
import re

_BENCHMARK_APP = 'https://core.mojoapps.io/benchmark.mojo'

# Additional time in seconds allocated per shell run to accommodate start-up.
# The shell should terminate before hitting this time out, it is an error if it
# doesn't.
_EXTRA_TIMEOUT = 20

_MEASUREMENT_RESULT_FORMAT = r"""
^                      # Beginning of the line.
measurement:           # Hard-coded tag.
\s+(.+)                # Match measurement spec.
\s+([0-9]+(.[0-9]+)?)  # Match measurement result.
$                      # End of the line.
"""

_MEASUREMENT_REGEX = re.compile(_MEASUREMENT_RESULT_FORMAT, re.VERBOSE)


def _parse_measurement_results(output):
  """Parses the measurement results present in the benchmark output and returns
  the dictionary of correctly recognized and parsed results.
  """
  measurement_results = {}
  output_lines = [line.strip() for line in output.split('\n')]
  for line in output_lines:
    match = re.match(_MEASUREMENT_REGEX, line)
    if match:
      measurement_spec = match.group(1)
      measurement_result = match.group(2)
      try:
        measurement_results[measurement_spec] = float(measurement_result)
      except ValueError:
        pass
  return measurement_results


class Outcome(object):
  """Holds results of a benchmark run."""

  def __init__(self, succeeded, error_str, output):
    self.succeeded = succeeded
    self.error_str = error_str
    self.output = output
    # Maps measurement specs to measurement results given as floats. Only
    # measurements that succeeded (ie. we retrieved their results) are
    # represented.
    self.results = {}
    self.some_measurements_failed = False


def run(shell, shell_args, app, duration_seconds, measurements, verbose,
        android, output_file):
  """Runs the given benchmark by running `benchmark.mojo` in mojo shell with
  appropriate arguments and returns the produced output.

  Returns:
    An instance of Outcome holding the results of the run.
  """
  timeout = duration_seconds + _EXTRA_TIMEOUT
  benchmark_args = []
  benchmark_args.append('--app=' + app)
  benchmark_args.append('--duration=' + str(duration_seconds))

  device_output_file = None
  if output_file:
    if android:
      device_output_file = os.path.join(shell.get_tmp_dir_path(), output_file)
      benchmark_args.append('--trace-output=' + device_output_file)
    else:
      benchmark_args.append('--trace-output=' + output_file)

  for measurement in measurements:
    benchmark_args.append(measurement['spec'])

  shell_args = list(shell_args)
  shell_args.append(_BENCHMARK_APP)
  shell_args.append('--force-offline-by-default')
  shell_args.append('--args-for=%s %s' % (_BENCHMARK_APP,
      ' '.join(map(pipes.quote, benchmark_args))))

  if verbose:
    print 'shell arguments: ' + str(shell_args)
  return_code, output, did_time_out = shell.run_and_get_output(
      shell_args, timeout=timeout)

  if did_time_out:
    return Outcome(False, 'timed out', output)
  if return_code:
    return Outcome(False, 'return code: ' + str(return_code), output)

  # Pull the trace file even if some measurements are missing, as it can be
  # useful in debugging.
  if device_output_file:
    shell.pull_file(device_output_file, output_file, remove_original=True)

  outcome = Outcome(True, None, output)
  parsed_results = _parse_measurement_results(output)
  for measurement in measurements:
    spec = measurement['spec']
    if spec in parsed_results:
      outcome.results[spec] = parsed_results[spec]
    else:
      outcome.some_measurements_failed = True
  return outcome
