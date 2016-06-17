// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <math.h>

#include "base/logging.h"
#include "services/media/audio/gain.h"

namespace mojo {
namespace media {
namespace audio {

constexpr unsigned int Gain::FRACTIONAL_BITS;
constexpr Gain::AScale Gain::UNITY;

Gain::Gain()
  : amplitude_scale_(UNITY) {
}

void Gain::Set(float db_gain) {
  // Compute the amplitude scale factor as a double and sanity check before
  // converting to the fixed point representation.
  double amp_scale = pow(10.0, db_gain / 20.0);
  DCHECK(amp_scale < (1u << ((sizeof(AScale) * 8) - FRACTIONAL_BITS)));

  amp_scale *= static_cast<double>(UNITY);
  amplitude_scale_.store(static_cast<AScale>(amp_scale));
}

}  // namespace audio
}  // namespace media
}  // namespace mojo

