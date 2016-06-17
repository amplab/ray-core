# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Disable the line-too-long warning.
# pylint: disable=C0301
"""This module implements the Chromium Performance Dashboard JSON v1.0 data
format.

See http://www.chromium.org/developers/speed-infra/performance-dashboard/sending-data-to-the-performance-dashboard.
"""

import httplib
import json
import pprint
import subprocess
import urllib
import urllib2


_LOCAL_SERVER = "http://127.0.0.1:8080"


class ChartDataRecorder(object):
  """Allows one to record measurement values one by one and then generate the
  JSON string that represents them in the 'chart_data' format expected by the
  performance dashboard.
  """

  def __init__(self, benchmark_name):
    self.charts = {}
    self.benchmark_name = benchmark_name

  def record_scalar(self, chart_name, value_name, units, value):
    """Records a single measurement value of a scalar type."""
    if chart_name not in self.charts:
      self.charts[chart_name] = {}
    self.charts[chart_name][value_name] = {
        'type': 'scalar',
        'units': units,
        'value': value}

  def record_vector(self, chart_name, value_name, units, values):
    """Records a single measurement value of a list of scalars type."""
    if chart_name not in self.charts:
      self.charts[chart_name] = {}
    self.charts[chart_name][value_name] = {
        'type': 'list_of_scalar_values',
        'units': units,
        'values': values}

  def get_chart_data(self):
    """Returns the JSON string representing the recorded chart data, wrapping
    it with the required meta data."""
    chart_data = {
        'format_version': '1.0',
        'benchmark_name': self.benchmark_name,
        'charts': self.charts
    }
    return chart_data


def add_argparse_server_arguments(parser):
  """Adds argparse arguments needed to upload the chart data to a performance
  dashboard to the given parser.
  """
  dashboard_group = parser.add_argument_group('Performance dashboard server',
      'These arguments allow to specify the performance dashboard server '
      'to upload the results to.')

  dashboard_group.add_argument(
      '--upload', action='store_true',
      help='Upload the results to performance dashboard. Further arguments '
           'in this group are relevant only if --upload is passed.')
  dashboard_group.add_argument(
      '--server-url',
      help='Url of the server instance to upload the results to. By default a '
           'local instance is assumed to be running on port 8080.')
  dashboard_group.add_argument(
      '--master-name',
      help='Buildbot master name, used to construct link to buildbot log by '
           'the dashboard, and also as the top-level category for the data.')
  dashboard_group.add_argument(
      '--bot-name',
      help='Used as the second-level category for the data.')
  dashboard_group.add_argument(
      '--test-name',
      help='Name of the test that the perf data was generated from.')
  dashboard_group.add_argument(
      '--builder-name',
      help='Buildbot builder name, used to construct link to buildbot log by '
           'the dashboard.')
  dashboard_group.add_argument(
      '--build-number', type=int,
      help='Build number, used to construct link to buildbot log by the '
           'dashboard.')
  dashboard_group.add_argument(
      '--dry-run', action='store_true', default=False,
      help='Display the server URL and the data to upload, but do not actually '
           'upload the data.')


def normalize_label(label):
  """Normalizes a label to be used for data sent to performance dashboard.

  This replaces:
    '/' -> '-', as slashes are used to denote test/sub-test relation.
    ' ' -> '_', as there is a convention of not using spaces in test names.

  Returns:
    Normalized label.
  """
  return label.replace('/', '-').replace(' ', '_')


def _get_commit_count():
  """Returns the number of git commits in the repository of the cwd."""
  return subprocess.check_output(
      ['git', 'rev-list', 'HEAD', '--count']).strip()


def _get_current_commit():
  """Returns the hash of the current commit in the repository of the cwd."""
  return subprocess.check_output(["git", "rev-parse", "HEAD"]).strip()


class _UploadException(Exception):
  pass


def _upload(server_url, json_data):
  """Make an HTTP POST with the given data to the performance dashboard.

  Args:
    server_url: URL of the performance dashboard instance.
    json_data: JSON string that contains the data to be sent.

  Raises:
    _UploadException: An error occurred during uploading.
  """
  # When data is provided to urllib2.Request, a POST is sent instead of GET.
  # The data must be in the application/x-www-form-urlencoded format.
  data = urllib.urlencode({"data": json_data})
  req = urllib2.Request("%s/add_point" % server_url, data)
  try:
    urllib2.urlopen(req)
  except urllib2.HTTPError as e:
    raise _UploadException('HTTPError: %d. Response: %s\n'
                           'JSON: %s\n' % (e.code, e.read(), json_data))
  except urllib2.URLError as e:
    raise _UploadException('URLError: %s for JSON %s\n' %
                           (str(e.reason), json_data))
  except httplib.HTTPException as e:
    raise _UploadException('HTTPException for JSON %s\n' % json_data)


def upload_chart_data(master_name, bot_name, test_name, builder_name,
                      build_number, chart_data, server_url=None, dry_run=False):
  """Uploads the provided chart data to an instance of performance dashboard.
  See the argparse help above for description of the arguments.


  Returns:
    A boolean value indicating whether the operation succeeded or not.
  """

  if (not master_name or not bot_name or not test_name or not builder_name or
      not build_number):
    print ('Cannot upload perf data to the dashboard because not all of the '
           'following values are specified: master-name, bot-name, test_name, '
           'builder-name, build-number.')
    return False

  point_id = _get_commit_count()
  cur_commit = _get_current_commit()

  # Wrap the |chart_data| with meta data as required by the spec.
  formatted_data = {
      'master': master_name,
      'bot': bot_name,
      'masterid': master_name,
      'buildername': builder_name,
      'buildnumber': build_number,
      'versions': {
          'mojo': cur_commit,
      },
      'point_id': point_id,
      'supplemental': {},
      'chart_data': chart_data,
  }

  upload_url = server_url if server_url else _LOCAL_SERVER

  if dry_run:
    print 'Will not upload because --dry-run is specified.'
    print 'Server: %s' % upload_url
    print 'Data:'
    pprint.pprint(formatted_data)
  else:
    print 'Uploading data to %s ...' % upload_url
    try:
      _upload(upload_url, json.dumps(formatted_data))
    except _UploadException as e:
      print e
      return False

    print "Done."

    dashboard_params = urllib.urlencode({
        'masters': master_name,
        'bots': bot_name,
        'tests': test_name,
        'rev': point_id
    })
    print 'Results Dashboard: %s/report?%s' % (upload_url, dashboard_params)

  return True
