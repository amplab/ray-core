#!/bin/bash
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Note: In the SDK, this script lives in mojo_sdk_setup.
SCRIPT_DIR=$(dirname $0)
DOWNLOADER=${SCRIPT_DIR}/download_file_from_google_storage.py

UNAME=$(uname)
case "$UNAME" in
  Linux)
    # TODO(vtl): We currently just always assume 64-bit.
    HOST_ARCH=Linux_x64
    ;;
  Darwin)
    HOST_ARCH=Mac
    ;;
  *)
    echo "$0: unknown system: ${UNAME}" 1>&2
    ;;
esac

CLANG_VERSION_FILE=${SCRIPT_DIR}/data/CLANG_VERSION
CLANG_VERSION=$(cat "$CLANG_VERSION_FILE")
CLANG_REVISION=259396
CLANG_SUB_REVISION=1
TAR_FILE=clang-${CLANG_REVISION}-${CLANG_SUB_REVISION}.tgz
GS_NAME=chromium-browser-clang/${HOST_ARCH}/${TAR_FILE}
OUT_DIR=${SCRIPT_DIR}/../toolchain/clang

DOWNLOAD_DIR=$(mktemp -d -t download_clang.XXXXXXXX)
function cleanup_download_dir {
  rm -rf "$DOWNLOAD_DIR"
}
trap cleanup_download_dir EXIT

"$DOWNLOADER" "$GS_NAME" "${DOWNLOAD_DIR}/${TAR_FILE}"
mkdir -p "$OUT_DIR"
tar xzfC "${DOWNLOAD_DIR}/${TAR_FILE}" "$OUT_DIR"
