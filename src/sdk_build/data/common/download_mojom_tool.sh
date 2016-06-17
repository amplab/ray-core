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
    HOST_ARCH=linux64
    ;;
  Darwin)
    HOST_ARCH=mac64
    ;;
  *)
    echo "$0: unknown system: ${UNAME}" 1>&2
    ;;
esac

FILE=${SCRIPT_DIR}/../mojo/public/tools/bindings/mojom_tool/bin/${HOST_ARCH}/mojom
HASH=$(cat "${FILE}.sha1")
# This includes the bucket name first.
GS_NAME=mojo/mojom_parser/${HOST_ARCH}/${HASH}

"$DOWNLOADER" --sha1-hash="${HASH}" --executable "$GS_NAME" "$FILE"
