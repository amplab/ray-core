#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Builds an SDK (in a specified target directory) from the (current) source
repository (which should not be dirty) and a given "specification" file."""


import argparse
from pylib.errors import FatalError
from pylib.git import Git, IsGitTreeDirty, SanityCheckGit, GitLsFiles
import os
import shutil
import sys


def _MakeDirs(*args, **kwargs):
  """Like |os.makedirs()|, but ignores |OSError| exceptions (it assumes that
  these are due to the directory already existing)."""
  try:
    os.makedirs(*args, **kwargs)
  except OSError:
    pass


def _CopyHelper(source_file, dest_file):
  """Copies |source_file| to |dest_file|. If |source_file| is user-executable,
  it will set |dest_file|'s mode to 0755 (user: rwx, group/other: rx);
  otherwise, it will set it to 0644 (user: rw, group/other: r)."""
  shutil.copy(source_file, dest_file)
  if os.stat(source_file).st_mode & 0100:
    os.chmod(dest_file, 0755)
  else:
    os.chmod(dest_file, 0644)


def _CopyDir(source_path, dest_path, **kwargs):
  """Copies directories from the source git repository (the current working
  directory should be the root of this repository) to the destination path.
  |source_path| and the keyword arguments are as for the arguments of
  |GitLsFiles|. |dest_path| should be the "root" destination path. Note that a
  file such as <source_path>/foo/bar/baz.quux is copied to
  <dest_path>/foo/bar/baz.quux."""

  # Normalize the source path. Note that this strips any trailing '/'.
  source_path = os.path.normpath(source_path)
  source_files = GitLsFiles(source_path, **kwargs)
  for source_file in source_files:
    rel_path = source_file[len(source_path) + 1:]
    dest_file = os.path.join(dest_path, rel_path)
    _MakeDirs(os.path.dirname(dest_file))
    _CopyHelper(source_file, dest_file)


def _CopyFiles(source_files, dest_path):
  """Copies a given source file or files from the source git repository to the
  given destination path (the current working directory should be the root of
  this repository) |source_files| should either be a relative path to a single
  file in the source git repository or an iterable of such paths; note that this
  does not check that files are actually in the git repository (i.e., are
  tracked)."""

  if type(source_files) is str:
    source_files = [source_files]
  _MakeDirs(dest_path)
  for source_file in source_files:
    _CopyHelper(source_file,
                os.path.join(dest_path, os.path.basename(source_file)))


def _GitGetRevision():
  """Returns the HEAD revision of the source git repository (the current working
  directory should be the root of this repository)."""

  return Git("rev-parse", "HEAD").strip()


def _ReadFile(source_file):
  """Reads the specified file from the source git repository (note: It does not
  verify that |source_file| is actually in the git repository, i.e., is tracked)
  and returns its contents."""

  with open(source_file, "rb") as f:
    return f.read()


def _WriteFile(dest_file, contents):
  """Writes the given contents to the specified destination file."""

  _MakeDirs(os.path.dirname(dest_file))
  with open(dest_file, "wb") as f:
    return f.write(contents)


def main():
  parser = argparse.ArgumentParser(
      description="Constructs an SDK from a specification.")
  parser.add_argument("--allow-dirty-tree", dest="allow_dirty_tree",
                      action="store_true",
                      help="proceed even if the source tree is dirty")
  parser.add_argument("sdk_spec_file", metavar="sdk_spec_file.sdk",
                      type=argparse.FileType("rb"),
                      help="spec file for the SDK to build")
  parser.add_argument("target_dir",
                      help="target directory (must not already exist)")
  args = parser.parse_args()

  target_dir = os.path.abspath(args.target_dir)

  # CD to the "src" directory (we should currently be in src/sdk_build).
  src_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
  os.chdir(src_dir)

  SanityCheckGit()

  if not args.allow_dirty_tree and IsGitTreeDirty():
    FatalError("tree appears to be dirty")

  # Set the umask, so that files/directories we create will be world- (and
  # group-) readable (and executable, for directories).
  os.umask(0022)

  try:
    os.mkdir(target_dir)
  except OSError:
    FatalError("failed to create target directory %s" % target_dir)

  # Wrappers to actually write things to the target directory:

  def CopyDirToTargetDir(source_path, rel_dest_path, **kwargs):
    return _CopyDir(source_path, os.path.join(target_dir, rel_dest_path),
                    **kwargs)

  def CopyFilesToTargetDir(source_files, rel_dest_path, **kwargs):
    return _CopyFiles(source_files, os.path.join(target_dir, rel_dest_path),
                      **kwargs)

  def WriteFileToTargetDir(rel_dest_file, contents):
    return _WriteFile(os.path.join(target_dir, rel_dest_file), contents)

  execution_globals = {"CopyDir": CopyDirToTargetDir,
                       "CopyFiles": CopyFilesToTargetDir,
                       "FatalError": FatalError,
                       "GitGetRevision": _GitGetRevision,
                       "GitLsFiles": GitLsFiles,
                       "ReadFile": _ReadFile,
                       "WriteFile": WriteFileToTargetDir}
  exec args.sdk_spec_file in execution_globals

  return 0


if __name__ == "__main__":
  sys.exit(main())
