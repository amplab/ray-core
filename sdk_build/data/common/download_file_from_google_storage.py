#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script for downloading a file from Google Cloud Storage."""


import argparse
import hashlib
import httplib
import os
import sys


def _FatalError(message):
  print >> sys.stderr, "%s: fatal error: %s" % (os.path.basename(sys.argv[0]),
                                                message)


def _DownloadFile(source):
  """Downloads |source| from Google Cloud Storage. |source| should be a path,
  including the bucket name (e.g., if the full URL is gs://mojo/a/b/c.xyz,
  |source| should be "mojo/a/b/c.xyz")."""

  try:
    conn = httplib.HTTPSConnection("storage.googleapis.com")
    conn.request("GET", "/" + source)
    resp = conn.getresponse()
    if resp.status != httplib.OK:
      _FatalError("HTTP status: %s" % resp.reason)
    data = resp.read()
    conn.close()
    return data
  except httplib.HTTPException as e:
    _FatalError("HTTP exception: %s" % str(e))


def main():
  parser = argparse.ArgumentParser(
      description="Downloads a file from Google Cloud Storage.")
  parser.add_argument("--sha1-hash", dest="sha1_hash",
                      help="SHA-1 hash for the downloaded file")
  parser.add_argument("--executable", action="store_true",
                      help="make the downloaded file executable")
  parser.add_argument("source", help="source path, including the bucket name")
  parser.add_argument("destination", help="destination path")
  args = parser.parse_args()

  bits = _DownloadFile(args.source)
  if args.sha1_hash:
    got_sha1_hash = hashlib.sha1(bits).hexdigest().lower()
    expected_sha1_hash = args.sha1_hash.lower()
    if got_sha1_hash != expected_sha1_hash:
      _FatalError("SHA-1 hash did not match: got %s, expected %s" %
                  (got_sha1_hash, expected_sha1_hash))

  with open(args.destination, "wb") as f:
    f.write(bits)

  if args.executable:
    curr_mode = os.stat(args.destination).st_mode
    # Set the x bits (0111) where the r bits (0444) are set.
    new_mode = curr_mode | ((curr_mode & 0444) >> 2)
    os.chmod(args.destination, new_mode)

  return 0


if __name__ == "__main__":
  sys.exit(main())
