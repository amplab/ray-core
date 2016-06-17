// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/media/framework/stages/util.h"

namespace mojo {
namespace media {

bool HasPositiveDemand(const std::vector<Output>& outputs) {
  for (const Output& output : outputs) {
    if (output.demand() == Demand::kPositive) {
      return true;
    }
  }

  return false;
}

}  // namespace media
}  // namespace mojo
