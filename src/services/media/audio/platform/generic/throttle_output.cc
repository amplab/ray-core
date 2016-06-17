// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "mojo/services/media/common/cpp/local_time.h"
#include "services/media/audio/audio_output_manager.h"
#include "services/media/audio/platform/generic/throttle_output.h"

namespace mojo {
namespace media {
namespace audio {

static constexpr LocalDuration TRIM_PERIOD = local_time::from_msec(10);

ThrottleOutput::ThrottleOutput(AudioOutputManager* manager)
  : StandardOutputBase(manager) {}

ThrottleOutput::~ThrottleOutput() {}

AudioOutputPtr ThrottleOutput::New(AudioOutputManager* manager) {
  return AudioOutputPtr(new ThrottleOutput(manager));
}

MediaResult ThrottleOutput::Init() {
  last_sched_time_ = LocalClock::now();
  return MediaResult::OK;
}

bool ThrottleOutput::StartMixJob(MixJob* job,
                                 const LocalTime& process_start) {
  // Compute our next callback time, and check to see if we are falling behind
  // in the process.
  last_sched_time_ += TRIM_PERIOD;
  if (process_start > last_sched_time_) {
    // TODO(johngro): We are falling behind on our trimming.  We should
    // probably tell someone.
    last_sched_time_ = process_start + TRIM_PERIOD;;
  }

  // TODO(johngro): We could optimize this trim operation by scheduling our
  // callback to the time at which the first pending packet in our queue will
  // end, instead of using this polling style.  This would have the addition
  // benefit of tighten the timing on returning packets (currently, we could
  // hold a packet for up to TRIM_PERIOD - episilon past its end pts before
  // releasing it)
  //
  // In order to do this, however, we would to wake up and recompute whenever
  // the rate transformations for one of our client tracks changes.  For now, we
  // just poll because its simpler.
  SetNextSchedTime(last_sched_time_);

  // The throttle output never actually mixes anything, it just provides
  // backpressure to the pipeline by holding references to AudioPackets until
  // after their presentation should be finished.  All we need to do here is
  // schedule our next callback to keep things running, and let the base class
  // implementation handle trimming the output.
  return false;
}

bool ThrottleOutput::FinishMixJob(const MixJob& job) {
  // Since we never start any jobs, this should never be called.
  DCHECK(false);
  return false;
}

}  // namespace audio
}  // namespace media
}  // namespace mojo

