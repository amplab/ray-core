// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_MEDIA_MODELS_DEMAND_H_
#define MOJO_MEDIA_MODELS_DEMAND_H_

namespace mojo {
namespace media {

// Expresses packet demand for signalling upstream in a graph.
enum class Demand {
  // Ordered such that (kNegative < kNeutral < kPositive).

  // No packet can currently be accepted.
  kNegative = -1,

  // A packet can be accepted but is not required to meet timing constraints.
  kNeutral = 0,

  // A packet is required to meet timing constraints.
  kPositive = 1
};

}  // namespace media
}  // namespace mojo

#endif  // MOJO_MEDIA_MODELS_DEMAND_H_
