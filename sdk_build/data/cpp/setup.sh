#!/bin/bash
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Note: In the SDK, this script lives in mojo_sdk_setup.
SCRIPT_DIR=$(dirname $0)

${SCRIPT_DIR}/download_mojom_tool.sh
${SCRIPT_DIR}/download_clang.sh
