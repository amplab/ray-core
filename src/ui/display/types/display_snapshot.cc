// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/types/display_snapshot.h"

namespace ui {

DisplaySnapshot::DisplaySnapshot(int64_t display_id,
                                 const gfx::Point& origin,
                                 const gfx::Size& physical_size,
                                 DisplayConnectionType type,
                                 const std::vector<const DisplayMode*>& modes,
                                 const DisplayMode* current_mode,
                                 const DisplayMode* native_mode)
    : display_id_(display_id),
      origin_(origin),
      physical_size_(physical_size),
      type_(type),
      modes_(modes),
      current_mode_(current_mode),
      native_mode_(native_mode),
      product_id_(kInvalidProductID) {
}

DisplaySnapshot::~DisplaySnapshot() {}

}  // namespace ui
