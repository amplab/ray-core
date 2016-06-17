# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Git support functions."""


from errors import FatalError
import fnmatch
import os.path
import subprocess


def Git(subcommand, *args):
  """Runs "git <subcommand> <args...>" and returns the output, including
  standard error. Raises |OSError| if it fails to run git at all and
  |subprocess.CalledProcessError| if the command returns failure (nonzero)."""

  return subprocess.check_output(
      ('git', subcommand) + args, stderr=subprocess.STDOUT)


def SanityCheckGit():
  """Does a sanity check that git is available and the current working directory
  is in a git repository (exits with an error message on failure)."""

  try:
    Git("status")
  except OSError:
    FatalError("failed to run git -- is it in your PATH?")
  except subprocess.CalledProcessError:
    FatalError("\"git status\" failed -- is %s in a git repository?" %
        os.getcwd())


def IsGitTreeDirty():
  """Checks if the tree looks dirty (returning true if it is)."""

  try:
    Git("diff-index", "--quiet", "HEAD")
  except subprocess.CalledProcessError:
    return True
  return False


def GitLsFiles(path, recursive=True, exclude_file_patterns=None,
              exclude_path_patterns=None):
  """Returns a list of files under the given path (the filenames will include
  the full path). If |recursive| is true (the default), then this list will
  include all files (recursively); if not, it will only include the files "at"
  the exact path. |exclude_file_patterns| may be a list of shell-style wildcards
  of filenames to exclude. Similarly, |exclude_path_patterns| may be a list of
  shell-style wildcards of paths to exclude (note: to make it easier to match
  subtrees, a trailing '/' is added to the path for matching purposes)."""

  # Files are "null-terminated". This results in an extra empty string at the
  # end. "Luckily", in the empty case, we also get an empty string.
  result = Git("ls-files", "-z", path).split("\0")[:-1]
  if not recursive:
    result = [f for f in result if os.path.dirname(f) == path]
  if exclude_file_patterns:
    for p in exclude_file_patterns:
      result = [f for f in result
                if not fnmatch.fnmatch(os.path.basename(f), p)]
  if exclude_path_patterns:
    for p in exclude_path_patterns:
      result = [f for f in result
                if not fnmatch.fnmatch(os.path.dirname(f) + "/", p)]
  return result
