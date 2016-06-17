// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>

#include "base/logging.h"
#include "services/media/audio/audio_track_impl.h"
#include "services/media/audio/audio_track_to_output_link.h"
#include "services/media/audio/platform/generic/mixer.h"
#include "services/media/audio/platform/generic/standard_output_base.h"

namespace mojo {
namespace media {
namespace audio {

static constexpr LocalDuration MAX_TRIM_PERIOD = local_time::from_msec(10);
constexpr uint32_t StandardOutputBase::MixJob::INVALID_GENERATION;

StandardOutputBase::TrackBookkeeping::TrackBookkeeping() {}
StandardOutputBase::TrackBookkeeping::~TrackBookkeeping() {}

StandardOutputBase::StandardOutputBase(AudioOutputManager* manager)
  : AudioOutput(manager) {
  setup_mix_ =
    [this] (const AudioTrackImplPtr& track, TrackBookkeeping* info) -> bool {
      return SetupMix(track, info);
    };

  process_mix_ =
    [this] (const AudioTrackImplPtr& track,
            TrackBookkeeping* info,
            const AudioPipe::AudioPacketRefPtr& pkt_ref) -> bool {
      return ProcessMix(track, info, pkt_ref);
    };

  setup_trim_ =
    [this] (const AudioTrackImplPtr& track, TrackBookkeeping* info) -> bool {
      return SetupTrim(track, info);
    };

  process_trim_ =
    [this] (const AudioTrackImplPtr& track,
            TrackBookkeeping* info,
            const AudioPipe::AudioPacketRefPtr& pkt_ref) -> bool {
      return ProcessTrim(track, info, pkt_ref);
    };

  next_sched_time_ = LocalClock::now();
  next_sched_time_known_ = true;
}

StandardOutputBase::~StandardOutputBase() {}

void StandardOutputBase::Process() {
  bool mixed = false;
  LocalTime now = LocalClock::now();

  // At this point, we should always know when our implementation would like to
  // be called to do some mixing work next.  If we do not know, then we should
  // have already shut down.
  //
  // If the next sched time has not arrived yet, don't attempt to mix anything.
  // Just trim the queues and move on.
  DCHECK(next_sched_time_known_);
  if (now >= next_sched_time_) {
    // Clear the flag, if the implementation does not set this flag by calling
    // SetNextSchedTime during the cycle, we consider it to be an error and shut
    // down.
    next_sched_time_known_ = false;

    // As long as our implementation wants to mix more and has not run into a
    // problem trying to finish the mix job, mix some more.
    do {
      ::memset(&cur_mix_job_, 0, sizeof(cur_mix_job_));

      if (!StartMixJob(&cur_mix_job_, now)) {
        break;
      }

      // If we have a mix job, then we must have an output formatter, and an
      // intermediate buffer allocated, and it must be large enough for the mix
      // job we were given.
      DCHECK(mix_buf_);
      DCHECK(output_formatter_);
      DCHECK_LE(cur_mix_job_.buf_frames, mix_buf_frames_);

      // Fill the intermediate buffer with silence.
      size_t bytes_to_zero = sizeof(int32_t)
                           * cur_mix_job_.buf_frames
                           * output_formatter_->channels();
      ::memset(mix_buf_.get(), 0, bytes_to_zero);

      // Mix each track into the intermediate buffer, then clip/format into the
      // final buffer.
      ForeachTrack(setup_mix_, process_mix_);
      output_formatter_->ProduceOutput(mix_buf_.get(),
                                       cur_mix_job_.buf,
                                       cur_mix_job_.buf_frames);

      mixed = true;
    } while (FinishMixJob(cur_mix_job_));
  }

  if (!next_sched_time_known_) {
    // TODO(johngro): log this as an error.
    ShutdownSelf();
    return;
  }

  // If we mixed nothing this time, make sure that we trim all of our track
  // queues.  No matter what is going on with the output hardware, we are not
  // allowed to hold onto the queued data past its presentation time.
  if (!mixed) {
    ForeachTrack(setup_trim_, process_trim_);
  }

  // Figure out when we should wake up to do more work again.  No matter how
  // long our implementation wants to wait, we need to make sure to wake up and
  // periodically trim our input queues.
  LocalTime max_sched_time = now + MAX_TRIM_PERIOD;
  ScheduleCallback((next_sched_time_ > max_sched_time)
                   ? max_sched_time
                   : next_sched_time_);
}

MediaResult StandardOutputBase::InitializeLink(
    const AudioTrackToOutputLinkPtr& link) {
  TrackBookkeeping* bk = AllocBookkeeping();
  AudioTrackToOutputLink::BookkeepingPtr ref(bk);

  // We should never fail to allocate our bookkeeping.  The only way this can
  // happen is if we have a badly behaved implementation.
  if (!bk) { return MediaResult::INTERNAL_ERROR; }

  // We cannot proceed if our track has somehow managed to go away already.
  AudioTrackImplPtr track = link->GetTrack();
  if (!track) { return MediaResult::INVALID_ARGUMENT; }

  // Pick a mixer based on the input and output formats.
  bk->mixer = Mixer::Select(track->Format(), output_formatter_
                                           ? &output_formatter_->format()
                                           : nullptr);
  if (bk->mixer == nullptr) { return MediaResult::UNSUPPORTED_CONFIG; }

  // Looks like things went well.  Stash a reference to our bookkeeping and get
  // out.
  link->output_bookkeeping() = std::move(ref);
  return MediaResult::OK;
}

StandardOutputBase::TrackBookkeeping* StandardOutputBase::AllocBookkeeping() {
  return new TrackBookkeeping();
}

void StandardOutputBase::SetupMixBuffer(uint32_t max_mix_frames) {
  DCHECK_GT(output_formatter_->channels(), 0u);
  DCHECK_GT(max_mix_frames, 0u);
  DCHECK_LE(max_mix_frames, std::numeric_limits<uint32_t>::max() /
                            output_formatter_->channels());

  mix_buf_frames_ = max_mix_frames;
  mix_buf_.reset(new int32_t[mix_buf_frames_ * output_formatter_->channels()]);
}

void StandardOutputBase::ForeachTrack(const TrackSetupTask& setup,
                                      const TrackProcessTask& process) {
  for (auto iter = links_.begin(); iter != links_.end(); ) {
    if (shutting_down()) { return; }

    // Is the track still around?  If so, process it.  Otherwise, remove the
    // track entry and move on.
    const AudioTrackToOutputLinkPtr& link = *iter;
    AudioTrackImplPtr track(link->GetTrack());

    auto tmp_iter = iter++;
    if (!track) {
      links_.erase(tmp_iter);
      continue;
    }

    // It would be nice to be able to use a dynamic cast for this, but currently
    // we are building with no-rtti
    TrackBookkeeping* info =
      static_cast<TrackBookkeeping*>(link->output_bookkeeping().get());
    DCHECK(info);

    // Make sure that the mapping between the track's frame time domain and
    // local time is up to date.
    info->UpdateTrackTrans(track);

    bool setup_done = false;
    AudioPipe::AudioPacketRefPtr pkt_ref;
    while (true) {
      // Try to grab the front of the packet queue.  If it has been flushed
      // since the last time we grabbed it, be sure to reset our mixer's
      // internal filter state.
      bool was_flushed;
      pkt_ref = link->LockPendingQueueFront(&was_flushed);
      if (was_flushed) {
        info->mixer->Reset();
      }

      // If the queue is empty, then we are done.
      if (!pkt_ref) { break; }

      // If we have not set up for this track yet, do so.  If the setup fails
      // for any reason, stop processing packets for this track.
      if (!setup_done) {
        setup_done = setup(track, info);
        if (!setup_done) { break; }
      }

      // Capture the amplitude to apply for the next bit of audio.
      info->amplitude_scale = link->amplitude_scale();

      // Now process the packet which is at the front of the track's queue.  If
      // the packet has been entirely consumed, pop it off the front and proceed
      // to the next one.  Otherwise, we are finished.
      if (!process(track, info, pkt_ref)) { break; }
      link->UnlockPendingQueueFront(&pkt_ref, true);
    }

    // Unlock the queue and proceed to the next track.
    link->UnlockPendingQueueFront(&pkt_ref, false);

    // Note: there is no point in doing this for the trim task, but it dosn't
    // hurt anything, and its easier then introducing another function to the
    // ForeachTrack arguments to run after each track is processed just for the
    // purpose of setting this flag.
    cur_mix_job_.accumulate = true;
  }
}

bool StandardOutputBase::SetupMix(const AudioTrackImplPtr& track,
                                  TrackBookkeeping* info) {
  // If we need to recompose our transformation from output frame space to input
  // fractional frames, do so now.
  DCHECK(info);
  info->UpdateOutputTrans(cur_mix_job_);
  cur_mix_job_.frames_produced = 0;

  return true;
}

bool StandardOutputBase::ProcessMix(
    const AudioTrackImplPtr& track,
    TrackBookkeeping* info,
    const AudioPipe::AudioPacketRefPtr& packet) {
  // Sanity check our parameters.
  DCHECK(info);
  DCHECK(packet);

  // We had better have a valid job, or why are we here?
  DCHECK(cur_mix_job_.buf_frames);
  DCHECK(cur_mix_job_.frames_produced <= cur_mix_job_.buf_frames);

  // We also must have selected a mixer, or we are in trouble.
  DCHECK(info->mixer);
  Mixer& mixer = *(info->mixer);

  // If this track is currently paused (or being sampled extremely slowly), our
  // step size will be zero.  We know that this packet will be relevant at some
  // point in the future, but right now it contributes nothing.  Tell the
  // ForeachTrack loop that we are done and to hold onto this packet for now.
  if (!info->step_size) {
    return false;
  }

  // Have we produced all that we are supposed to?  If so, hold the current
  // packet and move on to the next track.
  if (cur_mix_job_.frames_produced >= cur_mix_job_.buf_frames) {
    return false;
  }

  uint32_t frames_left = cur_mix_job_.buf_frames - cur_mix_job_.frames_produced;
  int32_t* buf = mix_buf_.get()
               + (cur_mix_job_.frames_produced * output_formatter_->channels());

  // Figure out where the first and last sampling points of this job are,
  // expressed in fractional track frames.
  int64_t first_sample_ftf;
  bool good = info->out_frames_to_track_frames.DoForwardTransform(
      cur_mix_job_.start_pts_of + cur_mix_job_.frames_produced,
      &first_sample_ftf);
  DCHECK(good);

  DCHECK(frames_left);
  int64_t final_sample_ftf = first_sample_ftf +
    ((frames_left - 1) * static_cast<int64_t>(info->step_size));

  // If the packet has no frames, there's no need to mix it and it may be
  // skipped.
  if (packet->end_pts() == packet->start_pts()) {
    return true;
  }

  // Figure out the PTS of the final frame of audio in our input packet.
  DCHECK((packet->end_pts() - packet->start_pts()) >= Mixer::FRAC_ONE);
  int64_t final_pts = packet->end_pts() - Mixer::FRAC_ONE;

  // If the PTS of the final frame of audio in our input is before the negative
  // window edge of our filter centered at our first sampling point, then this
  // packet is entirely in the past and may be skipped.
  if (final_pts < (first_sample_ftf - mixer.neg_filter_width())) {
    return true;
  }

  // If the PTS of the first frame of audio in our input is after the positive
  // window edge of our filter centered at our final sampling point, then this
  // packet is entirely in the future and should be held.
  if (packet->start_pts() > (final_sample_ftf + mixer.pos_filter_width())) {
    return false;
  }

  // Looks like the contents of this input packet intersect our mixer's filter.
  // Compute where in the output buffer the first sample will be produced, as
  // well as where, relative to the start of the input packet, this sample will
  // be taken from.
  int64_t input_offset_64 = first_sample_ftf - packet->start_pts();
  int64_t output_offset_64 = 0;
  int64_t first_sample_pos_window_edge = first_sample_ftf
                                       + mixer.pos_filter_width();

  // If the first frame in this packet comes after the positive edge of the
  // filter window, then we need to skip some number of output frames before
  // starting to produce data.
  if (packet->start_pts() > first_sample_pos_window_edge) {
    output_offset_64 = (packet->start_pts()
                     - first_sample_pos_window_edge
                     + info->step_size - 1) / info->step_size;
    input_offset_64 += output_offset_64 * info->step_size;
  }

  DCHECK_GE(output_offset_64, 0);
  DCHECK_LT(output_offset_64, static_cast<int64_t>(frames_left));
  DCHECK_LE(input_offset_64, std::numeric_limits<int32_t>::max());
  DCHECK_GE(input_offset_64, std::numeric_limits<int32_t>::min());

  uint32_t output_offset = static_cast<uint32_t>(output_offset_64);
  int32_t  frac_input_offset = static_cast<int32_t>(input_offset_64);

  // Looks like we are ready to go.  Iterate over the regions of this input
  // packet, mixing data as we go.
  size_t i;
  const auto& regions = packet->regions();
  for (i = 0; i < regions.size(); ++i) {
    const auto& region = regions[i];

    DCHECK_LE(region.frac_frame_len,
              static_cast<uint32_t>(std::numeric_limits<int32_t>::max()));

    if (frac_input_offset >= static_cast<int32_t>(region.frac_frame_len)) {
      frac_input_offset -= region.frac_frame_len;
      continue;
    }

    bool consumed_source = info->mixer->Mix(buf,
                                            frames_left,
                                            &output_offset,
                                            region.base,
                                            region.frac_frame_len,
                                            &frac_input_offset,
                                            info->step_size,
                                            info->amplitude_scale,
                                            cur_mix_job_.accumulate);
    DCHECK_LE(output_offset, frames_left);

    if (!consumed_source) {
      // Looks like we didn't consume all of this region.  Assert that we have
      // produced all of our frames and we are done.
      DCHECK(output_offset == frames_left);
      return false;
    }

    frac_input_offset -= region.frac_frame_len;
  }

  cur_mix_job_.frames_produced += output_offset;
  DCHECK_LE(cur_mix_job_.frames_produced, cur_mix_job_.buf_frames);
  DCHECK_EQ(i, regions.size());
  return true;
}

bool StandardOutputBase::SetupTrim(const AudioTrackImplPtr& track,
                                   TrackBookkeeping* info) {
  // Compute the cutoff time we will use to decide wether or not to trim
  // packets.  ForeachTracks has already updated our transformation, no need
  // for us to do so here.
  DCHECK(info);

  int64_t local_now_ticks = LocalClock::now().time_since_epoch().count();

  // The behavior of the RateControlBase implementation guarantees that the
  // transformation into the media timeline is never singular.  If the
  // forward transformation fails it can only be because of an overflow,
  // which should be impossible unless the user has defined a playback rate
  // where the ratio between media time ticks and local time ticks is
  // greater than one.
  //
  // IOW - this should never happen.  If it does, we just stop processing
  // payloads.
  //
  // TODO(johngro): Log an error?  Communicate this to the user somehow?
  if (!info->lt_to_track_frames.DoForwardTransform(local_now_ticks,
                                                  &trim_threshold_)) {
    return false;
  }

  return true;
}

bool StandardOutputBase::ProcessTrim(
    const AudioTrackImplPtr& track,
    TrackBookkeeping* info,
    const AudioPipe::AudioPacketRefPtr& pkt_ref) {
  DCHECK(pkt_ref);

  // If the presentation end of this packet is in the future, stop trimming.
  if (pkt_ref->end_pts() > trim_threshold_) {
    return false;
  }

  return true;
}

void StandardOutputBase::TrackBookkeeping::UpdateTrackTrans(
    const AudioTrackImplPtr& track) {
  LinearTransform tmp;
  uint32_t gen;

  DCHECK(track);
  track->SnapshotRateTrans(&tmp, &gen);

  // If the local time -> media time transformation has not changed since the
  // last time we examines it, just get out now.
  if (lt_to_track_frames_gen == gen) { return; }

  // The transformation has changed, re-compute the local time -> track frame
  // transformation.
  LinearTransform scale(0, track->FractionalFrameToMediaTimeRatio(), 0);
  bool good;

  lt_to_track_frames.a_zero = tmp.a_zero;
  good = scale.DoForwardTransform(tmp.b_zero, &lt_to_track_frames.b_zero);
  DCHECK(good);
  good = LinearTransform::Ratio::Compose(scale.scale,
                                         tmp.scale,
                                         &lt_to_track_frames.scale);
  DCHECK(good);

  // Update the generation, and invalidate the output to track generation.
  lt_to_track_frames_gen = gen;
  out_frames_to_track_frames_gen = MixJob::INVALID_GENERATION;
}

void StandardOutputBase::TrackBookkeeping::UpdateOutputTrans(
    const MixJob& job) {
  // We should not be here unless we have a valid mix job.  From our point of
  // view, this means that we have a job which supplies a valid transformation
  // from local time to output frames.
  DCHECK(job.local_to_output);
  DCHECK(job.local_to_output_gen != MixJob::INVALID_GENERATION);

  // If our generations match, we don't need to re-compute anything.  Just use
  // what we have already.
  if (out_frames_to_track_frames_gen == job.local_to_output_gen) { return; }

  // Assert that we have a good mapping from local time to fractional track
  // frames.
  //
  // TODO(johngro): Don't assume that 0 means invalid.  Make it a proper
  // constant defined somewhere.
  DCHECK(lt_to_track_frames_gen);

  // Compose the job supplied transformation from local to output with the
  // track supplied mapping from local to fraction input frames to produce a
  // transformation which maps from output frames to fractional input frames.
  //
  // TODO(johngro): Make this composition operation part of the LinearTransform
  // class instead of doing it by hand here.  Its a more complicated task that
  // one might initially think, because of the need to deal with the
  // intermediate offset term, and distributing it to either side of the end of
  // the transformation with a minimum amt of loss, while avoiding overflow.
  //
  // For now, we punt, do it by hand and just assume that everything went well.
  LinearTransform& dst = out_frames_to_track_frames;

  // Distribute the intermediate offset entirely to the fractional frame domain
  // for now.  We can do better by extracting portions of the intermedate
  // offset that can be scaled by the ratios on either side of with without
  // loss, but for now this should be close enough.
  int64_t intermediate = job.local_to_output->a_zero
                       - lt_to_track_frames.a_zero;
  int64_t track_frame_offset;

  // TODO(johngro): add routines to LinearTransform::Ratio which allow us to
  // scale using just a ratio without needing to create a linear transform with
  // empty offsets.
  LinearTransform tmp(0, lt_to_track_frames.scale, 0);
  bool good = tmp.DoForwardTransform(intermediate, &track_frame_offset);
  DCHECK(good);

  dst.a_zero = job.local_to_output->b_zero;
  dst.b_zero = lt_to_track_frames.b_zero + track_frame_offset;

  // TODO(johngro): Add options to allow us to invert one or both of the ratios
  // during composition instead of needing to make a temporary ratio to
  // acomplish the task.
  LinearTransform::Ratio tmp_ratio(job.local_to_output->scale.denominator,
                                   job.local_to_output->scale.numerator);
  good = LinearTransform::Ratio::Compose(tmp_ratio,
                                         lt_to_track_frames.scale,
                                         &dst.scale);;
  DCHECK(good);

  // Finally, compute the step size in fractional frames.  IOW, every time we
  // move forward one output frame, how many fractional frames of input do we
  // consume.  Don't bother doing the multiplication if we already know that the
  // numerator is zero.
  //
  // TODO(johngro): same complaint as before... Do this without a temp.  The
  // special casing should be handled in the routine added to
  // LinearTransform::Ratio.
  DCHECK(dst.scale.denominator);
  if (!dst.scale.numerator) {
    step_size = 0;
  } else {
    LinearTransform tmp(0, dst.scale, 0);
    int64_t tmp_step_size;

    good = tmp.DoForwardTransform(1, &tmp_step_size);

    DCHECK(good);
    DCHECK_GE(tmp_step_size, 0);
    DCHECK_LE(tmp_step_size, std::numeric_limits<uint32_t>::max());

    step_size = static_cast<uint32_t>(tmp_step_size);
  }

  // Done, update our generation.
  out_frames_to_track_frames_gen = job.local_to_output_gen;
}

}  // namespace audio
}  // namespace media
}  // namespace mojo
