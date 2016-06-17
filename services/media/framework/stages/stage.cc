// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "services/media/framework/stages/stage.h"

namespace mojo {
namespace media {

Stage::Stage() : in_supply_backlog_(false), in_demand_backlog_(false) {}

Stage::~Stage() {}

void Stage::UnprepareInput(size_t index) {}

void Stage::UnprepareOutput(size_t index, const UpstreamCallback& callback) {}

}  // namespace media
}  // namespace mojo
