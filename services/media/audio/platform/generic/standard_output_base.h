// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_AUDIO_PLATFORM_GENERIC_STANDARD_OUTPUT_BASE_H_
#define SERVICES_MEDIA_AUDIO_PLATFORM_GENERIC_STANDARD_OUTPUT_BASE_H_

#include "base/callback.h"
#include "mojo/services/media/common/cpp/linear_transform.h"
#include "mojo/services/media/common/cpp/local_time.h"
#include "mojo/services/media/common/interfaces/media_common.mojom.h"
#include "mojo/services/media/common/interfaces/media_types.mojom.h"
#include "services/media/audio/audio_output.h"
#include "services/media/audio/audio_track_to_output_link.h"
#include "services/media/audio/gain.h"
#include "services/media/audio/platform/generic/mixer.h"
#include "services/media/audio/platform/generic/output_formatter.h"

namespace mojo {
namespace media {
namespace audio {

class StandardOutputBase : public AudioOutput {
 public:
  ~StandardOutputBase() override;

 protected:
  struct MixJob {
    static constexpr uint32_t INVALID_GENERATION = 0;

    // State for the job set up once by the output implementation and then used
    // by all tracks.
    void*    buf;
    uint32_t buf_frames;
    int64_t  start_pts_of;  // start PTS, expressed in output frames.
    uint32_t local_to_output_gen;
    bool     accumulate;
    const LinearTransform* local_to_output;

    // State for the job which is set up for each track during SetupMix
    uint32_t frames_produced;
  };

  struct TrackBookkeeping : public AudioTrackToOutputLink::Bookkeeping {
    TrackBookkeeping();
    ~TrackBookkeeping() override;

    LinearTransform lt_to_track_frames;
    LinearTransform out_frames_to_track_frames;
    uint32_t lt_to_track_frames_gen = 0;
    uint32_t out_frames_to_track_frames_gen = MixJob::INVALID_GENERATION;
    uint32_t step_size;
    Gain::AScale amplitude_scale;
    MixerPtr mixer;

    void UpdateTrackTrans(const AudioTrackImplPtr& track);
    void UpdateOutputTrans(const MixJob& job);
  };

  explicit StandardOutputBase(AudioOutputManager* manager);

  void Process() final;
  MediaResult InitializeLink(const AudioTrackToOutputLinkPtr& link) final;

  void SetNextSchedTime(const LocalTime& next_sched_time) {
    next_sched_time_ = next_sched_time;
    next_sched_time_known_ = true;
  }

  void SetNextSchedDelay(const LocalDuration& next_sched_delay) {
    SetNextSchedTime(LocalClock::now() + next_sched_delay);
  }

  virtual bool StartMixJob(MixJob* job, const LocalTime& process_start) = 0;
  virtual bool FinishMixJob(const MixJob& job) = 0;
  virtual TrackBookkeeping* AllocBookkeeping();
  void SetupMixBuffer(uint32_t max_mix_frames);

  // Details about the final output format
  OutputFormatterPtr output_formatter_;

 private:
  using TrackSetupTask = std::function<bool(const AudioTrackImplPtr& track,
                                            TrackBookkeeping* info)>;
  using TrackProcessTask =
    std::function<bool(const AudioTrackImplPtr& track,
                       TrackBookkeeping* info,
                       const AudioPipe::AudioPacketRefPtr& pkt_ref)>;

  void ForeachTrack(const TrackSetupTask& setup,
                    const TrackProcessTask& process);

  bool SetupMix(const AudioTrackImplPtr& track, TrackBookkeeping* info);
  bool ProcessMix(const AudioTrackImplPtr& track,
                  TrackBookkeeping* info,
                  const AudioPipe::AudioPacketRefPtr& pkt_ref);

  bool SetupTrim(const AudioTrackImplPtr& track, TrackBookkeeping* info);
  bool ProcessTrim(const AudioTrackImplPtr& track,
                   TrackBookkeeping* info,
                   const AudioPipe::AudioPacketRefPtr& pkt_ref);

  LocalTime next_sched_time_;
  bool next_sched_time_known_;

  // State for the internal buffer which holds intermediate mix results.
  //
  // TODO(johngro): Right now, the cannonical intermediate format is signed 32
  // bit ints.  As time goes on, we may need to reconsider this.  This will
  // become more important when...
  //
  // 1) We support 24 bit audio.  Right now, with a 16 bit max, we can
  //    accumulate for up to a maximum of 2^16-1 tracks without needing to do
  //    anything special about about clipping.  With 24 bit audio, this number
  //    will drop to only 255 simultanious tracks.  It is unclear if this is a
  //    reasonable system-wide limitation or not.
  // 2) We support floating point audio.
  std::unique_ptr<int32_t[]> mix_buf_;
  uint32_t mix_buf_frames_ = 0;

  // State used by the mix task.
  TrackSetupTask setup_mix_;
  TrackProcessTask process_mix_;
  MixJob cur_mix_job_;

  // State used by the trim task.
  TrackSetupTask setup_trim_;
  TrackProcessTask process_trim_;
  int64_t trim_threshold_;
};

}  // namespace audio
}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_AUDIO_PLATFORM_GENERIC_STANDARD_OUTPUT_BASE_H_
