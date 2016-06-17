// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This provides a minimal factory for |SkFontMgr| (with no fonts available from
// the system). It uses |SkFontMgr_New_Custom_Embedded()| from
// third_party/skia/src/ports/SkFontMgr_custom.cpp.
//
// TODO(vtl): We should really have our own, even more minimal |SkFontMgr|
// implementation. SkFontMgr_custom.cpp includes the ability to gets fonts from
// disk (or find them from a directory), which we don't want/need, and
// necessitates bringing in third_party/skia/src/utils/SkOSFile.cpp.

#include "third_party/skia/include/ports/SkFontMgr.h"

// These are actually declared in
// third_party/skia/src/ports/SkFontMgr_custom.cpp (and not in any header). So
// we have to (partially) repeat the declarations.
struct SkEmbeddedResource;
struct SkEmbeddedResourceHeader {
  const SkEmbeddedResource* entries;
  int count;
};
SkFontMgr* SkFontMgr_New_Custom_Embedded(
  const SkEmbeddedResourceHeader* header);

// Static factory function declared in SkFontMgr.h.
// static
SkFontMgr* SkFontMgr::Factory() {
  static const SkEmbeddedResourceHeader kNoEmbeddedFonts = {nullptr, 0};
  return SkFontMgr_New_Custom_Embedded(&kNoEmbeddedFonts);
}
