// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/media/audio/fwd_decls.h"

namespace mojo {
namespace media {
namespace audio {

AudioOutputPtr CreateDefaultAlsaOutput(AudioOutputManager* manager) {
  return nullptr;
}

}  // namespace audio
}  // namespace media
}  // namespace mojo
