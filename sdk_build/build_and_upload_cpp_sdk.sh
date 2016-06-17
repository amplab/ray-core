#!/bin/bash
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script that "builds" and uploads a C++ SDK to GCS.

set -e

SCRIPT_DIR=$(dirname $0)
BUILD_SDK=${SCRIPT_DIR}/build_sdk.py

WORK_DIR=$(mktemp -d -t build_cpp_sdk.XXXXXXXX)
function cleanup_work_dir {
  rm -rf "$WORK_DIR"
}
trap cleanup_work_dir EXIT

# Build the SDK.
SDK_DIRNAME=mojo_cpp_sdk
SDK_DIR=${WORK_DIR}/${SDK_DIRNAME}
"$BUILD_SDK" "${SCRIPT_DIR}/data/cpp/cpp.sdk" "$SDK_DIR"

# Get the version (hash).
VERSION=$(cat "${SDK_DIR}/MOJO_SDK_VERSION")
# Abbreviate it to 12 hex digits.
SHORT_VERSION=${VERSION:0:12}
echo "SDK version (hash): ${VERSION} (${SHORT_VERSION})"

# Make a tarball.
TARBALL_FILENAME=mojo_cpp_sdk-${SHORT_VERSION}.tar.gz
TARBALL_FILE=${WORK_DIR}/${TARBALL_FILENAME}
# In a subshell, change to WORK_DIR so that the tarball will only have the
# mojo_cpp_sdk part of the path.
(
  cd "${WORK_DIR}"
  tar c --owner=root --group=root -z -f "$TARBALL_FILENAME" "$SDK_DIRNAME"
)

# Assume that we're the "latest". Make a file containing the abbreviated hash.
LATEST_FILENAME=LATEST
LATEST_FILE=${WORK_DIR}/${LATEST_FILENAME}
echo "$SHORT_VERSION" > "$LATEST_FILE"

echo "Press enter to upload ${TARBALL_FILENAME} to GCS"
read

GCS_BASE_URL="gs://mojo/sdk/cpp"
# Don't clobber existing tarballs.
gsutil cp -n "$TARBALL_FILE" "${GCS_BASE_URL}/${TARBALL_FILENAME}"
# But allow "LATEST" to be clobbered.
gsutil cp "$LATEST_FILE" "${GCS_BASE_URL}/${LATEST_FILENAME}"

echo "Done!"
