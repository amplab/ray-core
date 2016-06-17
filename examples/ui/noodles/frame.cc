// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ui/noodles/frame.h"

#include "base/logging.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPicture.h"

namespace examples {

Frame::Frame(const mojo::Size& size,
             sk_sp<SkPicture> picture,
             mojo::gfx::composition::SceneMetadataPtr scene_metadata)
    : size_(size), picture_(picture), scene_metadata_(scene_metadata.Pass()) {
  DCHECK(picture_);
}

Frame::~Frame() {}

void Frame::Paint(SkCanvas* canvas) {
  DCHECK(canvas);

  canvas->clear(SK_ColorBLACK);
  canvas->drawPicture(picture_.get());
  canvas->flush();
}

}  // namespace examples
