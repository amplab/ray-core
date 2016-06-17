// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/common/inprocess_messages.h"

namespace ui {

OzoneHostMsg_UpdateNativeDisplays::OzoneHostMsg_UpdateNativeDisplays(
  const std::vector<DisplaySnapshot_Params>& _displays)
  : Message(OZONE_HOST_MSG__UPDATE_NATIVE_DISPLAYS),
    displays(_displays) {
}

OzoneHostMsg_UpdateNativeDisplays::~OzoneHostMsg_UpdateNativeDisplays() {
}

} // namespace
