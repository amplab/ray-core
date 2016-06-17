// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_STAGES_ACTIVE_MULTISTREAM_SOURCE_STAGE_H_
#define SERVICES_MEDIA_FRAMEWORK_STAGES_ACTIVE_MULTISTREAM_SOURCE_STAGE_H_

#include <vector>

#include "base/synchronization/lock.h"
#include "services/media/framework/models/active_multistream_source.h"
#include "services/media/framework/stages/stage.h"

namespace mojo {
namespace media {

// A stage that hosts an ActiveMultistreamSource.
class ActiveMultistreamSourceStage : public Stage {
 public:
  ActiveMultistreamSourceStage(std::shared_ptr<ActiveMultistreamSource> source);

  ~ActiveMultistreamSourceStage() override;

  // Stage implementation.
  size_t input_count() const override;

  Input& input(size_t index) override;

  size_t output_count() const override;

  Output& output(size_t index) override;

  PayloadAllocator* PrepareInput(size_t index) override;

  void PrepareOutput(size_t index,
                     PayloadAllocator* allocator,
                     const UpstreamCallback& callback) override;

  void UnprepareOutput(size_t index, const UpstreamCallback& callback) override;

  void Update(Engine* engine) override;

  void FlushInput(size_t index, const DownstreamCallback& callback) override;

  void FlushOutput(size_t index) override;

 private:
  std::vector<Output> outputs_;
  std::shared_ptr<ActiveMultistreamSource> source_;
  ActiveMultistreamSource::SupplyCallback supply_function_;

  mutable base::Lock lock_;
  PacketPtr cached_packet_;
  size_t cached_packet_output_index_;
  size_t ended_streams_ = 0;
  bool packet_request_outstanding_ = false;
};

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_STAGES_ACTIVE_MULTISTREAM_SOURCE_STAGE_H_
