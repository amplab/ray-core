// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_VIEWPORT_DISPLAY_MANAGER_H_
#define SERVICES_NATIVE_VIEWPORT_DISPLAY_MANAGER_H_

#include <vector>
#include "base/memory/weak_ptr.h"
#include "ui/display/types/native_display_delegate.h"
#include "ui/display/types/native_display_observer.h"
#include "ui/gfx/geometry/rect.h"

namespace native_viewport {

class DisplayManager : public ui::NativeDisplayObserver {
 public:
  DisplayManager();
  ~DisplayManager() override;

 private:
  void OnDisplaysAcquired(const std::vector<ui::DisplaySnapshot*>& displays);
  void OnDisplayConfigured(const gfx::Rect& bounds, bool success);

  // ui::NativeDisplayObserver
  void OnConfigurationChanged() override;

  scoped_ptr<ui::NativeDisplayDelegate> delegate_;

  // Flags used to keep track of the current state of display configuration.
  //
  // True if configuring the displays. In this case a new display configuration
  // isn't started.
  bool is_configuring_;

  // If |is_configuring_| is true and another display configuration event
  // happens, the event is deferred. This is set to true and a display
  // configuration will be scheduled after the current one finishes.
  bool should_configure_;

  base::WeakPtrFactory<DisplayManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DisplayManager);
};

}  // namespace native_viewport

#endif
