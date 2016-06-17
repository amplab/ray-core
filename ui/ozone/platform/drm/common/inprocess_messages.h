// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_DRM_GPU_COMMON_INPROCESS_MESSAGES_H_
#define UI_OZONE_PLATFORM_DRM_HOST_DRM_GPU_COMMON_INPROCESS_MESSAGES_H_

#include "base/file_descriptor_posix.h"
#include "base/files/file_path.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"

namespace ui {

enum MessageId {
  OZONE_GPU_MSG__CREATE_WINDOW = 1000,
  OZONE_GPU_MSG__WINDOW_BOUNDS_CHANGED,
  OZONE_GPU_MSG__ADD_GRAPHICS_DEVICE,
  OZONE_GPU_MSG__REFRESH_NATIVE_DISPLAYS,
  OZONE_GPU_MSG__CONFIGURE_NATIVE_DISPLAY,

  OZONE_HOST_MSG__UPDATE_NATIVE_DISPLAYS = 2000,
  OZONE_HOST_MSG__DISPLAY_CONFIGURED,
  OZONE_HOST_MSG__HDCP_STATE_RECEIVED,
};

class Message {
 public:
  Message(MessageId _id)
    : id(_id) {
  }
  const MessageId id;
};

class OzoneGpuMsg_CreateWindow : public Message {
 public:
  OzoneGpuMsg_CreateWindow(const gfx::AcceleratedWidget& _widget)
    : Message(OZONE_GPU_MSG__CREATE_WINDOW),
      widget(_widget) {
  }
  const gfx::AcceleratedWidget widget;
};

class OzoneGpuMsg_WindowBoundsChanged : public Message {
 public:
  OzoneGpuMsg_WindowBoundsChanged(const gfx::AcceleratedWidget& _widget,
                                  const gfx::Rect& _bounds)
    : Message(OZONE_GPU_MSG__WINDOW_BOUNDS_CHANGED),
      widget(_widget), bounds(_bounds) {
  }
  const gfx::AcceleratedWidget widget;
  const gfx::Rect bounds;
};

class OzoneGpuMsg_AddGraphicsDevice : public Message {
 public:
  OzoneGpuMsg_AddGraphicsDevice(const base::FilePath& _path,
                                const base::FileDescriptor& _fd)
    : Message(OZONE_GPU_MSG__ADD_GRAPHICS_DEVICE),
      path(_path), fd(_fd) {
  }
  const base::FilePath path;
  const base::FileDescriptor fd;
};

class OzoneGpuMsg_RefreshNativeDisplays : public Message {
 public:
  OzoneGpuMsg_RefreshNativeDisplays()
    : Message(OZONE_GPU_MSG__REFRESH_NATIVE_DISPLAYS) {
  }
};

class OzoneGpuMsg_ConfigureNativeDisplay : public Message {
 public:
  OzoneGpuMsg_ConfigureNativeDisplay(int64_t _id,
                                     const DisplayMode_Params& _mode,
                                     const gfx::Point& _originhost)
    : Message(OZONE_GPU_MSG__CONFIGURE_NATIVE_DISPLAY)
    , id(_id), mode(_mode), originhost(_originhost) {
  }
  const int64_t id;
  const DisplayMode_Params mode;
  const gfx::Point originhost;
};


class OzoneHostMsg_UpdateNativeDisplays : public Message {
 public:
  OzoneHostMsg_UpdateNativeDisplays(
    const std::vector<DisplaySnapshot_Params>& _displays);
  ~OzoneHostMsg_UpdateNativeDisplays();

  const std::vector<DisplaySnapshot_Params> displays;
};

class OzoneHostMsg_DisplayConfigured : public Message {
 public:
  OzoneHostMsg_DisplayConfigured(int64_t _id, bool _result)
    : Message(OZONE_HOST_MSG__DISPLAY_CONFIGURED)
    , id(_id), result(_result) {
  }
  const int64_t id;
  const bool result;
};

} // namespace

#endif
