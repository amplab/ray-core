// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PUBLIC_IPC_INIT_HELPER_OZONE_H_
#define UI_OZONE_PUBLIC_IPC_INIT_HELPER_OZONE_H_

namespace ui {

class IpcInitHelperOzone {
 public:
  static IpcInitHelperOzone* Create();
};

}  // namespace ui

#endif
