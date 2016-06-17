# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys
import tempfile
import urllib2
import zipfile

from devtoolslib import paths


_SHELL_FILE_NAME = {
    'linux-x64': 'mojo_shell',
    'android-arm': 'MojoShell.apk'
}


def _get_artifacts_to_download(mojo_version, platform, verbose):
  """Returns a list of tuples of (gs_path, file_name) to be downloaded."""
  artifacts = []
  shell_gs_path = ('gs://mojo/shell/%s/%s.zip' % (mojo_version, platform))
  artifacts.append(
      (shell_gs_path, _SHELL_FILE_NAME[platform])
  )
  if platform == 'linux-x64':
    network_service_version_url = (
        'https://raw.githubusercontent.com/domokit/mojo/' +
        mojo_version + '/mojo/public/tools/NETWORK_SERVICE_VERSION')
    network_service_version = (
        urllib2.urlopen(network_service_version_url).read().strip())
    if verbose:
      print('Looked up the network service version for mojo at %s as: %s ' % (
          mojo_version, network_service_version))

    network_service_gs_path = (
        'gs://mojo/network_service/%s/%s/network_service.mojo.zip' %
        (network_service_version, platform))
    artifacts.append(
        (network_service_gs_path, 'network_service.mojo')
    )
  return artifacts


def _download_from_gs(gs_path, output_path, depot_tools_path, verbose):
  """Downloads the file at the given gs_path using depot_tools."""
  # We're downloading from a public bucket which does not need authentication,
  # but the user might have busted credential files somewhere such as ~/.boto
  # that the gsutil script will try (and fail) to use. Setting these
  # environment variables convinces gsutil not to attempt to use these.
  env = os.environ.copy()
  env['AWS_CREDENTIAL_FILE'] = ""
  env['BOTO_CONFIG'] = ""

  gsutil_exe = os.path.join(depot_tools_path, 'third_party', 'gsutil', 'gsutil')
  if verbose:
    print('Fetching ' + gs_path)

  try:
    subprocess.check_output(
        [gsutil_exe,
         '--bypass_prodaccess',
         'cp',
         gs_path,
         output_path],
        stderr=subprocess.STDOUT,
        env=env)
  except subprocess.CalledProcessError as e:
    print e.output
    sys.exit(1)


def _extract_file(archive_path, file_name, output_dir):
  with zipfile.ZipFile(archive_path) as z:
    zi = z.getinfo(file_name)
    mode = zi.external_attr >> 16
    z.extract(zi, output_dir)
    os.chmod(os.path.join(output_dir, file_name), mode)


def download_shell(mojo_version, platform, root_output_dir, verbose):
  """Downloads the shell (along with corresponding artifacts if needed) at the
  given version.
  """
  depot_tools_path = paths.find_depot_tools()
  artifacts = _get_artifacts_to_download(mojo_version, platform, verbose)
  output_dir = os.path.join(root_output_dir, mojo_version, platform)

  for (gs_path, file_name) in artifacts:
    if os.path.isfile(os.path.join(output_dir, file_name)):
      continue

    with tempfile.NamedTemporaryFile() as temp_zip_file:
      _download_from_gs(gs_path, temp_zip_file.name, depot_tools_path, verbose)
      _extract_file(temp_zip_file.name, file_name, output_dir)

  return os.path.join(output_dir, _SHELL_FILE_NAME[platform])
