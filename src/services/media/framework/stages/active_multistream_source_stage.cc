// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/media/framework/stages/active_multistream_source_stage.h"
#include "services/media/framework/stages/util.h"

namespace mojo {
namespace media {

ActiveMultistreamSourceStage::ActiveMultistreamSourceStage(
    std::shared_ptr<ActiveMultistreamSource> source)
    : source_(source) {
  DCHECK(source);
  outputs_.resize(source->stream_count());

  supply_function_ = [this](size_t output_index, PacketPtr packet) {
    lock_.Acquire();
    DCHECK(!cached_packet_) << "source supplied unrequested packet";
    DCHECK(output_index < outputs_.size());
    DCHECK(packet);
    DCHECK(packet_request_outstanding_);

    packet_request_outstanding_ = false;

    cached_packet_output_index_ = output_index;
    cached_packet_ = std::move(packet);

    if (cached_packet_->end_of_stream()) {
      ended_streams_++;
    }

    Output& output = outputs_[cached_packet_output_index_];
    if (output.demand() != Demand::kNegative) {
      lock_.Release();
      RequestUpdate();
    } else {
      lock_.Release();
    }
  };

  source_->SetSupplyCallback(supply_function_);
}

ActiveMultistreamSourceStage::~ActiveMultistreamSourceStage() {}

size_t ActiveMultistreamSourceStage::input_count() const {
  return 0;
};

Input& ActiveMultistreamSourceStage::input(size_t index) {
  CHECK(false) << "input requested from source";
  abort();
}

size_t ActiveMultistreamSourceStage::output_count() const {
  return outputs_.size();
}

Output& ActiveMultistreamSourceStage::output(size_t index) {
  DCHECK(index < outputs_.size());
  return outputs_[index];
}

PayloadAllocator* ActiveMultistreamSourceStage::PrepareInput(size_t index) {
  CHECK(false) << "PrepareInput called on source";
  return nullptr;
}

void ActiveMultistreamSourceStage::PrepareOutput(
    size_t index,
    PayloadAllocator* allocator,
    const UpstreamCallback& callback) {
  DCHECK(index < outputs_.size());

  if (allocator != nullptr) {
    // Currently, we don't support a source that uses provided allocators. If
    // we're provided an allocator, the output must have it so supplied packets
    // can be copied.
    outputs_[index].SetCopyAllocator(allocator);
  }
}

void ActiveMultistreamSourceStage::UnprepareOutput(
    size_t index,
    const UpstreamCallback& callback) {
  DCHECK(index < outputs_.size());
  outputs_[index].SetCopyAllocator(nullptr);
}

void ActiveMultistreamSourceStage::Update(Engine* engine) {
  base::AutoLock lock(lock_);
  DCHECK(engine);

  if (cached_packet_) {
    Output& output = outputs_[cached_packet_output_index_];
    if (output.demand() != Demand::kNegative) {
      // cached_packet_ is intended for an output which will accept packets.
      output.SupplyPacket(std::move(cached_packet_), engine);
    }
  }

  if (!cached_packet_ && HasPositiveDemand(outputs_) &&
      !packet_request_outstanding_) {
    // We have no cached packet and positive demand. Request a packet.
    source_->RequestPacket();
    packet_request_outstanding_ = true;
  }
}

void ActiveMultistreamSourceStage::FlushInput(
    size_t index,
    const DownstreamCallback& callback) {
  CHECK(false) << "FlushInput called on source";
}

void ActiveMultistreamSourceStage::FlushOutput(size_t index) {
  base::AutoLock lock(lock_);
  DCHECK(index < outputs_.size());
  DCHECK(source_);
  outputs_[index].Flush();
  cached_packet_.reset(nullptr);
  cached_packet_output_index_ = 0;
  ended_streams_ = 0;
  packet_request_outstanding_ = false;
}

}  // namespace media
}  // namespace mojo
