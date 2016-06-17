# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility functions for the stack tool."""

import glob
import os
import os.path
import re


def UnzipSymbols(symbolfile, symdir=None):
  """Unzips a file to _DEFAULT_SYMROOT and returns the unzipped location.

  Args:
    symbolfile: The .zip file to unzip
    symdir: Optional temporary directory to use for extraction

  Returns:
    A tuple containing (the directory into which the zip file was unzipped,
    the path to the "symbols" directory in the unzipped file).  To clean
    up, the caller can delete the first element of the tuple.

  Raises:
    SymbolDownloadException: When the unzip fails.
  """
  if not symdir:
    symdir = "%s/%s" % (_DEFAULT_SYMROOT, hash(symbolfile))
  if not os.path.exists(symdir):
    os.makedirs(symdir)

  print "extracting %s..." % symbolfile
  saveddir = os.getcwd()
  os.chdir(symdir)
  try:
    unzipcode = subprocess.call(["unzip", "-qq", "-o", symbolfile])
    if unzipcode > 0:
      os.remove(symbolfile)
      raise SymbolDownloadException("failed to extract symbol files (%s)."
                                    % symbolfile)
  finally:
    os.chdir(saveddir)

  android_symbols = glob.glob("%s/out/target/product/*/symbols" % symdir)
  if android_symbols:
    return (symdir, android_symbols[0])
  else:
    # This is a zip of Chrome symbols, so symbol.CHROME_SYMBOLS_DIR needs to be
    # updated to point here.
    symbol.CHROME_SYMBOLS_DIR = symdir
    return (symdir, symdir)


def GetBasenameFromMojoApp(url):
  """Used by GetSymbolMapping() to extract the basename from the location the
  mojo app was downloaded from. The location is a URL, e.g.
  http://foo/bar/x.so."""
  index = url.rfind('/')
  return url[(index + 1):] if index != -1 else url


def GetSymboledNameForMojoApp(path):
  """Used by GetSymbolMapping to get the non-stripped library name for an
  installed mojo app."""
  # e.g. tracing.mojo -> libtracing_library.so
  name, ext = os.path.splitext(path)
  if ext != '.mojo':
    return path
  return 'lib%s_library.so' % name


def GetSymbolMapping(lines):
  """Returns a mapping (dictionary) from download file to .so."""
  regex = re.compile('Caching mojo app (\S+?)(?:\?\S+)? at (\S+)')
  mappings = {}
  for line in lines:
    result = regex.search(line)
    if result:
      url = GetBasenameFromMojoApp(result.group(1))
      path = os.path.normpath(result.group(2))
      name = GetSymboledNameForMojoApp(url)
      mappings[path] = name
      if path.startswith('/data/user/0/'):
        mappings[path.replace('/data/user/0/', '/data/data/', 1)] = name
  return mappings


def _LowestAncestorContainingRelpath(dir_path, relpath):
  """Returns the lowest ancestor dir of |dir_path| that contains |relpath|.
  """
  cur_dir_path = os.path.abspath(dir_path)
  while True:
    if os.path.exists(os.path.join(cur_dir_path, relpath)):
      return cur_dir_path

    next_dir_path = os.path.dirname(cur_dir_path)
    if next_dir_path != cur_dir_path:
      cur_dir_path = next_dir_path
    else:
      return None


def GuessDir(relpath):
  """Returns absolute path to location |relpath| in the lowest ancestor of this
  file that contains it."""
  lowest_ancestor = _LowestAncestorContainingRelpath(
      os.path.dirname(__file__), relpath)
  if not lowest_ancestor:
    return None
  return os.path.join(lowest_ancestor, relpath)
