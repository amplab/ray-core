# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Library module for building go binaries used by build_mojom_tool and
build_generators.
"""

import argparse
import imp
import os
import shutil
import subprocess
import sys


class GoBinaryBuilder(object):
  _TARGET_ARCHITECTURES = [
      ('darwin', 'amd64'),
      ('linux', 'amd64'),
      ]

  def __init__(self,
      target_dir,
      binary_name,
      src_path,
      src_root=None,
      go_tool=None,
      go_root=None,
      out_dir=None,
      upload=False,
      friendly_name=None,
      quiet=False,
      keep_going=False):
    """Builds a go binary building object.

    Args:
      target_dir: Relative path from the architecture directory to the
          directory which contains the sha1 file.
      binary_name: File name for the binary to be built.
      src_path: Path from src_root to the directory which contains the main
          package for the binary to be built.
      src_root: Path to the root of the source tree. (If None, the path of the
          directory 3 levels above the one containing this script is used.)
      go_tool: Path to the go tool.
          (If None, uses {src_root}/third_party/go/tool/linux_amd64/bin/go)
      go_root: Path to the root of the go installation.
          (If None, uses the directory containing the go tool.)
      out_dir: Path to the directory in which the built binary should be placed.
          (If None, uses the current working directory.)
      upload: If True, get the path to the depot tools as those are needed to
          upload only. If False, the upload method will fail.
      quiet: If True, do not print any messages on success.
      keep_going: If True, move on to the next step when experiencing a failure.
    """
    self._target_dir = target_dir
    self._binary_name = binary_name
    self._src_path = src_path
    if src_root:
      self._src_root = os.path.abspath(src_root)
    else:
      self._src_root = os.path.abspath(
          os.path.join(os.path.dirname(__file__), *([os.pardir] * 2)))
    assert os.path.exists(self._src_root)

    if go_tool:
      self._go_tool = os.path.abspath(go_tool)
    else:
      self._go_tool = os.path.join(self._src_root,
          'third_party', 'go', 'tool', 'linux_amd64', 'bin', 'go')
    assert os.path.exists(self._go_tool)

    if go_root:
      self._go_root = os.path.abspath(go_root)
    else:
      self._go_root = os.path.dirname(os.path.dirname(self._go_tool))
    assert os.path.exists(self._go_root)

    if not out_dir:
      out_dir = os.getcwd()
    self._out_dir = os.path.abspath(out_dir)

    self._depot_tools_path = None
    if upload:
      self._depot_tools_path = self._get_depot_tools_path(self._src_root)

    self._quiet = quiet
    self._keep_going = keep_going

    self._friendly_name = friendly_name
    if self._friendly_name is None:
      self._friendly_name = binary_name

  def build_and_upload(self):
    """Builds and then uploads the binary for every target architecture.

    Returns:
      0 if everything went well. The return code of the last command to fail
      otherwise.
    """
    final_result = 0

    # First we build the binary for every architecture.
    result = self.build()
    if result != 0:
      final_result = result
      if not self._keep_going:
        return result

    # Then we upload the binary for every architecture.
    result = self.upload()
    if result != 0:
      final_result = result
      if not self._keep_going:
        return result

    return final_result

  def build(self):
    """Builds the binary for every target architectures.

    Returns:
      0 if everything went well. The return code of the last command to fail
      otherwise.
    """
    final_result = 0
    for target_os, target_arch in self._TARGET_ARCHITECTURES:
      result = self._build_for_arch(target_os, target_arch)
      if result != 0:
        final_result = result
        if not self._keep_going:
          return result

    return final_result

  def upload(self):
    """Uploads the binary that was just created for every target arch.

    Returns:
      0 if everything went well. The return code of the last command to fail
      otherwise.
    """
    final_result = 0
    for target_os, target_arch in self._TARGET_ARCHITECTURES:
      result = self._upload_for_arch(target_os, target_arch)
      if result != 0:
        final_result = result
        if not self._keep_going:
          return result
    return final_result

  def _upload_for_arch(self, target_os, target_arch):
    """Uploads the binary that was just created for the given target.

    upload must have been set to True when creating this class.

    Args:
      target_os: Any value valid for GOOS.
      target_arch: Any value valid for GOARCH.

    Returns:
      0 if everything went well, something else otherwise.
    """
    assert self._depot_tools_path
    upload_result = self._upload_binary(target_os, target_arch)
    if upload_result != 0:
      return upload_result

    # Copy the generated sha1 file to the stamp_file location.
    target_dir = os.path.join(
        self._get_dir_name_for_arch(target_os, target_arch),
        self._target_dir)
    sha1_file = '%s.sha1' % self._get_output_path(target_os, target_arch)
    assert os.path.exists(sha1_file)
    stamp_file = os.path.join(self._src_root, 'mojo', 'public', 'tools',
        'bindings', 'mojom_tool', 'bin', target_dir,
        '%s.sha1' % self._binary_name)
    shutil.move(sha1_file, stamp_file)
    self._info_print(
        "Wrote stamp file %s. You probably want to commit this." % stamp_file)

    return 0

  def _build_for_arch(self, target_os, target_arch):
    """Builds the mojom parser for target_os and target_arch.

    Args:
      target_os: Any value valid for GOOS.
      target_arch: Any value valid for GOARCH.

    Returns:
      The go tool's return value.
    """
    out_path = self._get_output_path(target_os, target_arch)

    environ = {
        'GOROOT': self._go_root,
        'GOPATH': os.path.dirname(self._src_root),
        'GOOS': target_os,
        'GOARCH': target_arch,
        }

    save_cwd = os.getcwd()
    os.chdir(os.path.join(self._src_root, self._src_path))
    self._info_print(
        "Building the Mojom parser for %s %s..." % (target_os, target_arch))
    result = subprocess.call(
        [self._go_tool, 'build', '-o', out_path] , env=environ)
    if result == 0:
      self._info_print("Success! Built %s" % out_path)
    else:
      print >> sys.stderr, "Failure building Mojom parser for %s %s!" % (
          target_os, target_arch)

    os.chdir(save_cwd)

    return result

  def _upload_binary(self, target_os, target_arch):
    """Computes the sha1 digest of the contents of the specified binary, then
    uploads the contents of the file at that path to the Google Cloud Storage
    bucket "mojo" using the filename "mojom_tool/$%target/$sha1." $target
    is computed based upon the target_os and target_arch. A file whose name is
    the name of the binary with .sha1 appended is created containing the sha1.

    This method will only work if self._depot_tools_path has been set.
    """
    assert self._depot_tools_path
    upload_script = os.path.abspath(os.path.join(self._depot_tools_path,
        "upload_to_google_storage.py"))
    out_path = self._get_output_path(target_os, target_arch)
    assert os.path.exists(out_path)


    cloud_path = 'mojo/mojom_parser/%s' % self._get_dir_name_for_arch(
        target_os, target_arch)
    if self._target_dir:
      cloud_path = '%s/%s' % (cloud_path, self._target_dir)

    upload_cmd = ['python', upload_script, out_path, '-b', cloud_path]

    stdout = None
    if self._quiet:
      stdout = open(os.devnull, 'w')
    self._info_print(
        "Uploading %s (%s,%s) to GCS..." %
        (self._friendly_name, target_os, target_arch))
    return subprocess.call(upload_cmd, stdout=stdout)

  def _get_dir_name_for_arch(self, target_os, target_arch):
    dir_names = {
        ('darwin', 'amd64'): 'mac64',
        ('linux', 'amd64'): 'linux64',
        }
    return dir_names[(target_os, target_arch)]

  def _get_output_path(self, target_os, target_arch):
    return os.path.join(
        self._out_dir, '%s_%s_%s' % (self._binary_name, target_os, target_arch))

  def _get_depot_tools_path(self, src_root):
    name = 'find_depot_tools'
    find_depot_tools = imp.load_source(name,
        os.path.join(src_root, 'tools', name + '.py'))
    return find_depot_tools.add_depot_tools_to_path()

  def _info_print(self, message):
    if self._quiet:
      return
    print message


def get_arg_parser(description):
  """Returns an ArgumentParser instance with the provided description and with
  required arguments already added.
  """
  parser = argparse.ArgumentParser(description=description)
  parser.add_argument('--src-root', dest='src_root', action='store',
                      help='Path to the source tree. (optional)')
  parser.add_argument('--go-tool', dest='go_tool', action='store',
                      help='Path to the go tool. (optional)')
  parser.add_argument('--go-root', dest='go_root', action='store',
                      help='Path to the root of the go installation. '
                           '(optional)')
  parser.add_argument('--out-dir', dest='out_dir', action='store',
                      help='Directory in which to place the built binary. '
                           'Defaults to the current working directory. '
                           '(optional)')
  parser.add_argument('--upload', dest='upload', action='store_true',
                      default=False,
                      help='Upload the built binaries to Google Cloud Storage '
                           'and update the corresponding hash files locally.')
  parser.add_argument('--keep-going', dest='keep_going', action='store_true',
                      default=False,
                      help='Instead of stopping when encountering a failure '
                           'move on to the next step.')
  parser.add_argument('--quiet', dest='quiet', action='store_true',
                      default=False,
                      help='Do not output anything on success.')
  return parser


def get_builder(args, target_dir, binary_name, src_path, friendly_name=None):
  """Return a GoBinaryBuilder object."""
  return GoBinaryBuilder(
      target_dir=target_dir,
      binary_name=binary_name,
      src_path=src_path,
      src_root=args.src_root,
      go_tool=args.go_tool,
      go_root=args.go_root,
      out_dir=args.out_dir,
      upload=args.upload,
      quiet=args.quiet,
      keep_going=args.keep_going,
      friendly_name=friendly_name)

