// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_STAGES_ACTIVE_MULTISTREAM_SINK_STAGE_H_
#define SERVICES_MEDIA_FRAMEWORK_STAGES_ACTIVE_MULTISTREAM_SINK_STAGE_H_

#include <list>
#include <set>
#include <vector>

#include "base/synchronization/lock.h"
#include "services/media/framework/models/active_multistream_sink.h"
#include "services/media/framework/stages/stage.h"

namespace mojo {
namespace media {

// A stage that hosts an ActiveSink.
class ActiveMultistreamSinkStage : public Stage,
                                   public ActiveMultistreamSinkHost {
 public:
  ActiveMultistreamSinkStage(std::shared_ptr<ActiveMultistreamSink> sink);

  ~ActiveMultistreamSinkStage() override;

  // Stage implementation.
  size_t input_count() const override;

  Input& input(size_t index) override;

  size_t output_count() const override;

  Output& output(size_t index) override;

  PayloadAllocator* PrepareInput(size_t index) override;

  void PrepareOutput(size_t index,
                     PayloadAllocator* allocator,
                     const UpstreamCallback& callback) override;

  void Update(Engine* engine) override;

  void FlushInput(size_t index, const DownstreamCallback& callback) override;

  void FlushOutput(size_t index) override;

  // ActiveMultistreamSinkHost implementation.
  size_t AllocateInput() override;

  size_t ReleaseInput(size_t index) override;

  void UpdateDemand(size_t input_index, Demand demand) override;

 private:
  struct StageInput {
    StageInput(size_t index)
        : index_(index), allocated_(false), demand_(Demand::kNegative) {}
    Input input_;
    size_t index_;
    bool allocated_;
    Demand demand_;
  };

  std::shared_ptr<ActiveMultistreamSink> sink_;

  mutable base::Lock lock_;
  std::vector<std::unique_ptr<StageInput>> inputs_;
  std::set<size_t> unallocated_inputs_;
  std::list<size_t> pending_inputs_;
};

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_STAGES_ACTIVE_MULTISTREAM_SINK_STAGE_H_
