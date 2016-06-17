// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>
#include <set>

#include "mojo/services/media/common/cpp/local_time.h"
#include "services/media/audio/platform/linux/alsa_output.h"

namespace mojo {
namespace media {
namespace audio {

constexpr LocalDuration AlsaOutput::kChunkDuration;
constexpr uint32_t AlsaOutput::kLowBufThreshChunks;
constexpr uint32_t AlsaOutput::kTargetLatencyChunks;
constexpr LocalDuration AlsaOutput::kLowBufThresh;
constexpr LocalDuration AlsaOutput::kTargetLatency;

static constexpr LocalDuration kErrorRecoveryTime = local_time::from_msec(300);
static constexpr LocalDuration kWaitForAlsaDelay  = local_time::from_usec(500);
static const std::set<uint8_t> kSupportedChannelCounts({ 1, 2 });
static const std::set<uint32_t> kSupportedSampleRates({
    48000, 32000, 24000, 16000, 8000, 4000,
    44100, 22050, 11025,
});

static inline bool IsRecoverableAlsaError(int error_code) {
  switch (error_code) {
  case -EINTR:
  case -EPIPE:
  case -ESTRPIPE:
    return true;
  default:
    return false;
  }
}

AudioOutputPtr CreateDefaultAlsaOutput(AudioOutputManager* manager) {
  // TODO(johngro): Do better than this.  If we really want to support
  // Linux/ALSA as a platform, we should be creating one output for each
  // physical output in the system, matching our configuration to the physical
  // output's configuration, and disabling resampling at the ALSA level.
  //
  // If we could own the output entirely and bypass the mixer to achieve lower
  // latency, that would be even better.
  AudioOutputPtr audio_out(audio::AlsaOutput::New(manager));
  if (!audio_out) { return nullptr; }

  AlsaOutput* alsa_out = static_cast<AlsaOutput*>(audio_out.get());
  DCHECK(alsa_out);

  AudioMediaTypeDetailsPtr config(AudioMediaTypeDetails::New());
  config->frames_per_second = 48000;
  config->channels = 2;
  config->sample_format = AudioSampleFormat::SIGNED_16;

  if (alsa_out->Configure(config.Pass()) != MediaResult::OK) {
    return nullptr;
  }

  return audio_out;
}

AlsaOutput::AlsaOutput(AudioOutputManager* manager)
  : StandardOutputBase(manager) {}

AlsaOutput::~AlsaOutput() {
  // We should have been cleaned up already, but in release builds, call cleanup
  // anyway, just in case something got missed.
  DCHECK(!alsa_device_);
  Cleanup();
}

AudioOutputPtr AlsaOutput::New(AudioOutputManager* manager) {
  return AudioOutputPtr(new AlsaOutput(manager));
}

MediaResult AlsaOutput::Configure(AudioMediaTypeDetailsPtr config) {
  if (!config) { return MediaResult::INVALID_ARGUMENT; }
  if (output_formatter_) { return MediaResult::BAD_STATE; }

  output_formatter_ = OutputFormatter::Select(config);
  if (!output_formatter_) { return MediaResult::UNSUPPORTED_CONFIG; }

  MediaResult res = AlsaSelectFormat(config);
  if (res != MediaResult::OK) { return res; }
  DCHECK_GE(alsa_format_, 0);

  if (kSupportedSampleRates.find(config->frames_per_second) ==
      kSupportedSampleRates.end()) {
    return MediaResult::UNSUPPORTED_CONFIG;
  }

  if (kSupportedChannelCounts.find(config->channels) ==
      kSupportedChannelCounts.end()) {
    return MediaResult::UNSUPPORTED_CONFIG;
  }

  // Compute the ratio between frames and local time ticks.
  LinearTransform::Ratio sec_per_tick(LocalDuration::period::num,
                                      LocalDuration::period::den);
  LinearTransform::Ratio frames_per_sec(config->frames_per_second, 1);
  bool is_precise = LinearTransform::Ratio::Compose(frames_per_sec,
                                                    sec_per_tick,
                                                    &frames_per_tick_);
  DCHECK(is_precise);

  // Success
  return MediaResult::OK;
}

MediaResult AlsaOutput::Init() {
  if (!output_formatter_) { return MediaResult::BAD_STATE; }
  if (alsa_device_) { return MediaResult::BAD_STATE; }

  MediaResult res = AlsaOpen();
  if (res != MediaResult::OK) {
    Cleanup();
    return res;
  }
  DCHECK(mix_buf_frames_);

  size_t buffer_size;
  buffer_size = mix_buf_frames_ * output_formatter_->bytes_per_frame();
  mix_buf_.reset(new uint8_t[buffer_size]);

  // Set up the intermediate buffer at the StandardOutputBase level
  SetupMixBuffer(mix_buf_frames_);

  return MediaResult::OK;
}

void AlsaOutput::Cleanup() {
  AlsaClose();
  DCHECK(!alsa_device_);

  mix_buf_ = nullptr;
  mix_buf_frames_ = 0;
}

bool AlsaOutput::StartMixJob(MixJob* job, const LocalTime& process_start) {
  DCHECK(job);
  DCHECK(alsa_device_);

  // Are we not primed?  If so, fill a mix buffer with silence and send it to
  // the alsa device.  Schedule a callback for a short time in the future so
  // ALSA has a chance to start the output and we can take our best guess of the
  // function which maps output frames to local time.
  if (!primed_) {
    HandleAsUnderflow();
    return false;
  }

  // Figure out how many frames of audio we need to produce in order to top off
  // the buffer.  If we are primed, but do not know the transformation between
  // audio frames and local time ticks, do our best to figure it out in the
  // process.
  int res;
  uint32_t avail;
  if (!local_to_output_known_) {
    uint32_t delay;
    res = AlsaGetAvailDelay(&avail, &delay);
    LocalTime now = LocalClock::now();

    // When using the FNL tinyalsa implementation, if we have queued enough data
    // to start, but have not actually started yet, the implementation will
    // return EAGAIN to indicate that we are going to start "Real Soon Now".
    // Wait just a bit, then try again.
    if (res == -EAGAIN) {
      SetNextSchedDelay(local_time::from_msec(1));
      return false;
    }

    if (res < 0) {
      HandleAlsaError(res);
      return false;
    }

    int64_t now_ticks = now.time_since_epoch().count();
    local_to_output_ = LinearTransform(now_ticks,
                                       frames_per_tick_,
                                       -static_cast<int64_t>(delay));
    local_to_output_known_ = true;
    frames_sent_ = 0;
    while (++local_to_output_gen_ == MixJob::INVALID_GENERATION) {}
  } else {
    res = AlsaGetAvailDelay(&avail);
    if (res < 0) {
      HandleAlsaError(res);
      return false;
    }
  }

  // Compute the time that we think we will completely underflow, then back off
  // from that by the low buffer threshold and use that to determine when we
  // should mix again.
  int64_t playout_time_ticks;
  bool trans_ok = local_to_output_.DoReverseTransform(frames_sent_,
                                                      &playout_time_ticks);
  DCHECK(trans_ok);
  LocalTime playout_time = LocalTime(LocalDuration(playout_time_ticks));
  LocalTime low_buf_time = playout_time - kLowBufThresh;

  if (process_start >= low_buf_time) {
    // Because of the way that ALSA consumes data and updates its internal
    // bookkeeping, it is possible that we are past our low buffer threshold,
    // but ALSA still thinks that there is no room to write new frames.  If this
    // is the case, just try again a short amount of time in the future.
    if (!avail) {
      SetNextSchedDelay(kWaitForAlsaDelay);
      return false;
    }

    // Limit the amt that we queue to be no more than what ALSA will currently
    // accept, or what it currently will take to fill us to our target latency.
    //
    // The playout target had better be ahead of the playout time, or we are
    // almost certainly going to underflow.  If this happens, for whatever
    // reason, just try to send a full buffer and deal with the underflow when
    // ALSA notices it.
    int64_t fill_amt;
    LocalTime now = LocalClock::now();
    LocalTime playout_target = now + kTargetLatency;
    if (playout_target > playout_time) {
      fill_amt = (playout_target - playout_time).count();
    } else {
      fill_amt = kTargetLatency.count();
    }

    DCHECK_GE(fill_amt, 0);
    fill_amt *= frames_per_tick_.numerator;
    fill_amt += frames_per_tick_.denominator - 1;
    fill_amt /= frames_per_tick_.denominator;

    job->buf_frames = (avail < fill_amt) ? avail : fill_amt;
    if (job->buf_frames > mix_buf_frames_) {
      job->buf_frames = mix_buf_frames_;
    }

    job->buf = mix_buf_.get();
    job->start_pts_of = frames_sent_;
    job->local_to_output = &local_to_output_;
    job->local_to_output_gen = local_to_output_gen_;

    return true;
  }

  // Wait until its time to mix some more data.
  SetNextSchedTime(low_buf_time);
  return false;
}

bool AlsaOutput::FinishMixJob(const MixJob& job) {
  DCHECK(job.buf == mix_buf_.get());
  DCHECK(job.buf_frames);

  // We should always be able to write all of the data that we mixed.
  int res;
  res = AlsaWrite(job.buf, job.buf_frames);
  if (static_cast<unsigned int>(res) != job.buf_frames) {
    HandleAlsaError(res);
    return false;
  }

  frames_sent_ += res;
  return true;
}

void AlsaOutput::HandleAsUnderflow() {
  int res;

  // If we were already primed, then this is a legitimate underflow, not the
  // startup case or recovery from some other error.
  if (primed_) {
    // TODO(johngro): come up with a way to properly throttle this.  Also, add a
    // friendly name to the output so the log helps to identify which output
    // underflowed.
    LOG(WARNING) << "[" << this << "] : underflow";
    res = AlsaRecover(-EPIPE);
    if (res < 0) {
      HandleAsError(res);
      return;
    }
  }

  // TODO(johngro): We don't actually have to fill up the entire lead time with
  // silence.  When we have better control of our thread priorities, prime this
  // with the minimimum amt we can get away with and still be able to start
  // mixing without underflowing.
  output_formatter_->FillWithSilence(mix_buf_.get(), mix_buf_frames_);
  res = AlsaWrite(mix_buf_.get(), mix_buf_frames_);

  if (res < 0) {
    HandleAsError(res);
    return;
  }

  primed_ = true;
  local_to_output_known_ = false;
  SetNextSchedDelay(local_time::from_msec(1));
}

void AlsaOutput::HandleAsError(int code) {
  if (IsRecoverableAlsaError(code)) {
    // TODO(johngro): Throttle this somehow.
    LOG(WARNING) << "[" << this << "] : Attempting to recover from ALSA error "
                 << code;

    int new_code = AlsaRecover(code);
    DCHECK(!new_code || (new_code == code));

    // If we recovered, or we didn't and the original error was EINTR, schedule
    // a retry time in the future and unwind.
    //
    // TODO(johngro): revisit the topic of errors we fail to snd_pcm_recover
    // from.  If we cannot recover from them, we should probably close and
    // re-open the device.  No matter what, we should put some form of limit on
    // how many times we try before really giving up and shutting down the
    // output for good.  We also need to invent a good way to test these edge
    // cases.
    if (!new_code || (new_code == -EINTR)) {
      primed_ = false;
      local_to_output_known_ = false;
      SetNextSchedDelay(kErrorRecoveryTime);
    }
  }

  LOG(ERROR) << "[" << this << "] : Fatal ALSA error "
             << code << ".  Shutting down";
  ShutdownSelf();
}

void AlsaOutput::HandleAlsaError(int code) {
  // ALSA signals an underflow by returning -EPIPE from jobs.  If the error code
  // is -EPIPE, treat this as an underflow and attempt to reprime the pipeline.
  if (code == -EPIPE) {
    HandleAsUnderflow();
  } else {
    HandleAsError(code);
  }
}

}  // namespace audio
}  // namespace media
}  // namespace mojo
