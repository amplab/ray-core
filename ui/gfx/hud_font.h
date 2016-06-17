// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_HUD_FONT_H_
#define UI_GFX_HUD_FONT_H_

#include "third_party/skia/include/core/SkRefCnt.h"
#include "ui/gfx/gfx_export.h"

class SkTypeface;

namespace gfx {

GFX_EXPORT void SetHudTypeface(sk_sp<SkTypeface> typeface);
GFX_EXPORT sk_sp<SkTypeface> GetHudTypeface();

}  // namespace gfx

#endif  // UI_GFX_HUD_FONT_H_
