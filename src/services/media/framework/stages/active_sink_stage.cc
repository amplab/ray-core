// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/media/framework/stages/active_sink_stage.h"

namespace mojo {
namespace media {

ActiveSinkStage::ActiveSinkStage(std::shared_ptr<ActiveSink> sink)
    : sink_(sink) {
  DCHECK(sink_);

  demand_function_ = [this](Demand demand) {
    if (sink_demand_ != demand) {
      sink_demand_ = demand;
      RequestUpdate();
    }
  };

  sink_->SetDemandCallback(demand_function_);
}

ActiveSinkStage::~ActiveSinkStage() {}

size_t ActiveSinkStage::input_count() const {
  return 1;
};

Input& ActiveSinkStage::input(size_t index) {
  DCHECK_EQ(index, 0u);
  return input_;
}

size_t ActiveSinkStage::output_count() const {
  return 0;
}

Output& ActiveSinkStage::output(size_t index) {
  CHECK(false) << "output requested from sink";
  abort();
}

PayloadAllocator* ActiveSinkStage::PrepareInput(size_t index) {
  DCHECK_EQ(index, 0u);
  return sink_->allocator();
}

void ActiveSinkStage::PrepareOutput(size_t index,
                                    PayloadAllocator* allocator,
                                    const UpstreamCallback& callback) {
  CHECK(false) << "PrepareOutput called on sink";
}

void ActiveSinkStage::Update(Engine* engine) {
  DCHECK(engine);
  DCHECK(sink_);

  if (input_.packet_from_upstream()) {
    sink_demand_ =
        sink_->SupplyPacket(std::move(input_.packet_from_upstream()));
  }

  input_.SetDemand(sink_demand_, engine);
}

void ActiveSinkStage::FlushInput(size_t index,
                                 const DownstreamCallback& callback) {
  DCHECK(sink_);
  input_.Flush();
  sink_->Flush();
  sink_demand_ = Demand::kNegative;
}

void ActiveSinkStage::FlushOutput(size_t index) {
  CHECK(false) << "FlushOutput called on sink";
}

}  // namespace media
}  // namespace mojo
