#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tool to roll Chromium into Mojo. See:
https://github.com/domokit/mojo/wiki/Rolling-code-between-chromium-and-mojo#chromium---mojo-updates
"""

import argparse
import glob
import itertools
import os
import subprocess
import sys
import tempfile
import time
import zipfile

import mopy.gn as gn
from mopy.config import Config
from mopy.paths import Paths
from mopy.version import Version

sys.path.append(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir,
                             "third_party", "pyelftools"))
import elftools.elf.elffile as elffile

sys.path.append(os.path.join(os.path.dirname(__file__), os.pardir,
                             'devtools', 'common'))
import android_gdb.signatures as signatures


BLACKLISTED_APPS = [
  # The network service apps are not produced out of the Mojo repo, but may
  # be present in the build dir.
  "network_service.mojo",
  "network_service_apptests.mojo",
]


def target(config):
  target_name = config.target_os + "-" + config.target_cpu
  if config.is_official_build:
    target_name += "-official"
  return target_name


def find_apps_to_upload(build_dir):
  apps = []
  for path in glob.glob(build_dir + "/*"):
    if not os.path.isfile(path):
      continue
    _, ext = os.path.splitext(path)
    if ext != '.mojo':
      continue
    if os.path.basename(path) in BLACKLISTED_APPS:
      continue
    apps.append(path)
  return apps


def check_call(config, command_line, dry_run, **kwargs):
  if dry_run:
    print command_line
  else:
    if config.values['verbose']:
      print command_line
    subprocess.check_call(command_line, **kwargs)


def upload(config, source, dest, dry_run, gzip=False):
  if config.values['use_default_path_for_gsutil']:
    gsutil_exe = "gsutil"
  else:
    paths = Paths(config)
    sys.path.insert(0, os.path.join(paths.src_root, "tools"))
    # pylint: disable=F0401
    import find_depot_tools
    depot_tools_path = find_depot_tools.add_depot_tools_to_path()
    gsutil_exe = os.path.join(
                            depot_tools_path, "third_party", "gsutil", "gsutil")

  command_line = [gsutil_exe, "cp"]
  if gzip and "." in source:
    extension = source.split(".")[-1]
    command_line.extend(["-z", extension])
  command_line.extend([source, dest])

  check_call(config, command_line, dry_run)


def upload_symbols(config, build_dir, breakpad_upload_urls, dry_run):
  dump_syms_exe = os.path.join(Paths().src_root,
                               "mojo", "tools", "linux64", "dump_syms")
  symupload_exe = os.path.join(Paths().src_root,
                               "mojo", "tools", "linux64", "symupload")
  dest_dir = config.values['upload_location'] + "symbols/"
  symbols_dir = os.path.join(build_dir, "symbols")
  with open(os.devnull, "w") as devnull:
    for name in os.listdir(symbols_dir):
      path = os.path.join(symbols_dir, name)
      with open(path) as f:
        signature = signatures.get_signature(f, elffile)
        if signature is not None:
          dest = dest_dir + signature
          upload(config, path, dest, dry_run)
      if breakpad_upload_urls:
        with tempfile.NamedTemporaryFile() as temp:
          check_call(config, [dump_syms_exe, path], dry_run,
                     stdout=temp, stderr=devnull)
          temp.flush()
          for upload_url in breakpad_upload_urls:
            check_call(config, [symupload_exe, temp.name, upload_url], dry_run)

def upload_shell(config, dry_run, verbose):
  paths = Paths(config)
  zipfile_name = target(config)
  version = Version().version

  # Upload the binary.
  # TODO(blundell): Change this to be in the same structure as the LATEST files,
  # e.g., gs://mojo/shell/linux-x64/<version>/shell.zip.
  dest = config.values['upload_location'] + "shell/" + version + "/" + \
         zipfile_name + ".zip"
  with tempfile.NamedTemporaryFile() as zip_file:
    with zipfile.ZipFile(zip_file, 'w') as z:
      for filename in paths.target_mojo_shell_binaries:
        with open(filename) as binary:
          basename = os.path.basename(filename)
          zipinfo = zipfile.ZipInfo(basename)
          zipinfo.external_attr = 0777 << 16L
          compress_type = zipfile.ZIP_DEFLATED
          if config.target_os == Config.OS_ANDROID:
            # The APK is already compressed.
            compress_type = zipfile.ZIP_STORED
          zipinfo.compress_type = compress_type
          zipinfo.date_time = time.gmtime(os.path.getmtime(filename))
          if verbose:
            print "zipping %s" % filename
          z.writestr(zipinfo, binary.read())
    upload(config, zip_file.name, dest, dry_run, gzip=False)

  # Update the LATEST file to contain the version of the new binary.
  latest_file = config.values['upload_location'] + \
                ("shell/%s/LATEST" % target(config))
  write_file_to_gs(version, latest_file, config, dry_run)


def upload_dart_snapshotter(config, dry_run, verbose):
  # Only built for Linux and Mac.
  if not config.target_os in [Config.OS_LINUX, Config.OS_MAC]:
    return

  paths = Paths(config)
  zipfile_name = target(config)
  version = Version().version

  dest = config.values['upload_location'] + "dart_snapshotter/" + version + \
         "/" + zipfile_name + ".zip"
  with tempfile.NamedTemporaryFile() as zip_file:
    with zipfile.ZipFile(zip_file, 'w') as z:
      dart_snapshotter_path = paths.target_dart_snapshotter_path
      with open(dart_snapshotter_path) as dart_snapshotter_binary:
        dart_snapshotter_filename = os.path.basename(dart_snapshotter_path)
        zipinfo = zipfile.ZipInfo(dart_snapshotter_filename)
        zipinfo.external_attr = 0777 << 16L
        compress_type = zipfile.ZIP_DEFLATED
        zipinfo.compress_type = compress_type
        zipinfo.date_time = time.gmtime(os.path.getmtime(dart_snapshotter_path))
        if verbose:
          print "zipping %s" % dart_snapshotter_path
        z.writestr(zipinfo, dart_snapshotter_binary.read())
    upload(config, zip_file.name, dest, dry_run, gzip=False)

  # Update the LATEST file to contain the version of the new binary.
  latest_file = config.values['upload_location'] + \
                ("dart_snapshotter/%s/LATEST" % target(config))
  write_file_to_gs(version, latest_file, config, dry_run)


def upload_app(app_binary_path, config, dry_run):
  app_binary_name = os.path.basename(app_binary_path)
  version = Version().version
  gsutil_app_location = (config.values['upload_location'] + \
      ("services/%s/%s/%s" % (target(config), version, app_binary_name)))

  # Upload the new binary.
  upload(config, app_binary_path, gsutil_app_location, dry_run)


def upload_system_thunks_lib(config, dry_run):
  paths = Paths(config)
  version = Version().version
  dest = config.values['upload_location'] + \
         ("system_thunks/%s/%s/libsystem_thunks.a" % (target(config), version))
  source_path = paths.build_dir + "/obj/mojo/libsystem_thunks.a"
  upload(config, source_path, dest, dry_run)


def write_file_to_gs(file_contents, dest, config, dry_run):
  with tempfile.NamedTemporaryFile() as temp_version_file:
    temp_version_file.write(file_contents)
    temp_version_file.flush()
    upload(config, temp_version_file.name, dest, dry_run)


def main():
  parser = argparse.ArgumentParser(description="Upload binaries for apps and "
      "the Mojo shell to google storage (by default on Linux, but this can be "
      "changed via options).")
  parser.add_argument("-n", "--dry_run", help="Dry run, do not actually "+
      "upload", action="store_true")
  parser.add_argument("-v", "--verbose", help="Verbose mode",
      action="store_true")
  parser.add_argument("--android",
                      action="store_true",
                      help="Upload the shell and apps for Android")
  parser.add_argument("--official",
                      action="store_true",
                      help="Upload the official build of the Android shell")
  parser.add_argument("-p", "--use-default-path-for-gsutil",
                      action="store_true",
                      help=("Use default $PATH location for finding gsutil."
        "  Helpful when gcloud sdk has been installed and $PATH has been set."))
  parser.add_argument("--symbols-upload-url",
                      action="append", default=[],
                      help="URL of the server to upload breakpad symbols to")
  parser.add_argument("-u", "--upload-location",
                      default="gs://mojo/",
                      help="root URL of the server to upload binaries to")
  args = parser.parse_args()
  is_official_build = args.official
  target_os = None
  if args.android:
    target_os = Config.OS_ANDROID
  elif is_official_build:
    print "Uploading official builds is only supported for android."
    return 1

  config = Config(target_os=target_os, is_debug=False,
                  is_official_build=is_official_build,
                  upload_location=args.upload_location,
                  use_default_path_for_gsutil=args.use_default_path_for_gsutil,
                  verbose=args.verbose)

  if not is_official_build:
    upload_dart_snapshotter(config, args.dry_run, args.verbose)

  if config.target_os == Config.OS_MAC:
    print "Skipping uploading apps and shell (mac build)."
    return 0

  upload_shell(config, args.dry_run, args.verbose)

  if is_official_build:
    print "Skipping uploading apps (official apk build)."
    return 0

  build_directory = gn.BuildDirectoryForConfig(config, Paths(config).src_root)
  apps_to_upload = find_apps_to_upload(build_directory)
  for app in apps_to_upload:
    upload_app(app, config, args.dry_run)

  upload_symbols(config, build_directory,
                 args.symbols_upload_url, args.dry_run)

  upload_system_thunks_lib(config, args.dry_run)

  return 0

if __name__ == "__main__":
  sys.exit(main())
