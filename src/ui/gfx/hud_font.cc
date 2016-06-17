// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/hud_font.h"

#include "base/lazy_instance.h"
#include "third_party/skia/include/core/SkTypeface.h"

namespace gfx {

namespace {
base::LazyInstance<sk_sp<SkTypeface>> g_hud_typeface;
}  // namespace

void SetHudTypeface(sk_sp<SkTypeface> typeface) {
  g_hud_typeface.Get() = typeface;
}

sk_sp<SkTypeface> GetHudTypeface() {
  // nullptr is fine; caller will create its own in that case.
  return g_hud_typeface.Get();
}

}  // namespace gfx
