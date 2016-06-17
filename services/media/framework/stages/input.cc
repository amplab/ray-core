// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/media/framework/engine.h"
#include "services/media/framework/stages/input.h"
#include "services/media/framework/stages/stage.h"

namespace mojo {
namespace media {

Input::Input() : prepared_(false) {}

Input::~Input() {}

void Input::Connect(const OutputRef& output) {
  DCHECK(output.valid());
  DCHECK(!mate_);
  mate_ = output;
}

Output& Input::actual_mate() const {
  DCHECK(mate_.valid());
  return mate_.actual();
}

void Input::SetDemand(Demand demand, Engine* engine) const {
  DCHECK(engine);
  DCHECK(connected());

  if (actual_mate().UpdateDemandFromInput(demand)) {
    engine->PushToDemandBacklog(mate().stage_);
  }
}

bool Input::SupplyPacketFromOutput(PacketPtr packet) {
  DCHECK(packet);
  DCHECK(!packet_from_upstream_);
  packet_from_upstream_ = std::move(packet);
  return true;
}

void Input::Flush() {
  packet_from_upstream_.reset(nullptr);
}

}  // namespace media
}  // namespace mojo
