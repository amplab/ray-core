// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_AUDIO_PLATFORM_LINUX_ALSA_OUTPUT_H_
#define SERVICES_MEDIA_AUDIO_PLATFORM_LINUX_ALSA_OUTPUT_H_

#include <memory>

#include "mojo/services/media/common/cpp/linear_transform.h"
#include "mojo/services/media/common/interfaces/media_types.mojom.h"
#include "services/media/audio/audio_output.h"
#include "services/media/audio/platform/generic/standard_output_base.h"

namespace mojo {
namespace media {
namespace audio {

class AlsaOutput : public StandardOutputBase {
 public:
  static AudioOutputPtr New(AudioOutputManager* manager);
  ~AlsaOutput() override;

  MediaResult Configure(AudioMediaTypeDetailsPtr config);

 protected:
  explicit AlsaOutput(AudioOutputManager* manager);

  // AudioOutput implementation
  MediaResult Init() override;
  void Cleanup() override;

  // StandardOutputBase implementation
  bool StartMixJob(MixJob* job, const LocalTime& process_start) override;
  bool FinishMixJob(const MixJob& job) override;

 private:
  static constexpr LocalDuration kChunkDuration  = local_time::from_msec(5);
  static constexpr uint32_t kLowBufThreshChunks  = 6;
  static constexpr uint32_t kTargetLatencyChunks = kLowBufThreshChunks + 1;
  static constexpr LocalDuration kLowBufThresh   = kChunkDuration
                                                 * kLowBufThreshChunks;
  static constexpr LocalDuration kTargetLatency  = kChunkDuration
                                                 * kTargetLatencyChunks;

  void HandleAlsaError(int code);
  void HandleAsError(int code);
  void HandleAsUnderflow();

  // libtinyalsa vs. libasound abstraction.  Methods are implemented either in
  // alsa_output_tinyalsa.cc or alsa_output_desktop.cc depending on the target
  // we are building for.
  MediaResult AlsaOpen();
  void AlsaClose();
  MediaResult AlsaSelectFormat(const AudioMediaTypeDetailsPtr& config);
  int AlsaWrite(const void* data, uint32_t frames);
  int AlsaGetAvailDelay(uint32_t* avail, uint32_t* delay = nullptr);
  int AlsaRecover(int err_code);

  void* alsa_device_ = nullptr;
  int32_t alsa_format_ = -1;

  LinearTransform::Ratio frames_per_tick_;

  std::unique_ptr<uint8_t[]> mix_buf_;
  uint32_t mix_buf_frames_ = 0;

  bool primed_ = false;
  bool local_to_output_known_ = false;
  LinearTransform local_to_output_;
  uint32_t local_to_output_gen_ = MixJob::INVALID_GENERATION + 1;
  int64_t frames_sent_;
};

}  // namespace audio
}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_AUDIO_PLATFORM_LINUX_ALSA_OUTPUT_H_
