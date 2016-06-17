// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"

#include "services/media/audio/audio_output.h"
#include "services/media/audio/audio_output_manager.h"
#include "services/media/audio/audio_track_to_output_link.h"

namespace mojo {
namespace media {
namespace audio {

// Function used in a base::Closure to defer the final part of a shutdown task
// to the main event loop.
static void FinishShutdownSelf(AudioOutputManager* manager,
                               AudioOutputWeakPtr weak_output) {
  auto output  = weak_output.lock();
  if (output) {
    manager->ShutdownOutput(output);
  }
}

AudioOutput::AudioOutput(AudioOutputManager* manager)
  : manager_(manager) {
  DCHECK(manager_);
}

AudioOutput::~AudioOutput() {
  DCHECK(!task_runner_ && shutting_down_);
}

MediaResult AudioOutput::AddTrackLink(AudioTrackToOutputLinkPtr link) {
  MediaResult res = InitializeLink(link);

  if (res == MediaResult::OK) {
    base::AutoLock lock(processing_lock_);

    // Assert that we are the output in this link.
    DCHECK(this == link->GetOutput().get());

    if (shutting_down_) {
      return MediaResult::SHUTTING_DOWN;
    }

    auto insert_result = links_.emplace(link);
    DCHECK(insert_result.second);
  } else {
    // TODO(johngro): Output didn't like this track for some reason...  Should
    // probably log something about this.
  }

  return res;
}

MediaResult AudioOutput::RemoveTrackLink(
    const AudioTrackToOutputLinkPtr& link) {
  base::AutoLock lock(processing_lock_);

  if (shutting_down_) {
    return MediaResult::SHUTTING_DOWN;
  }

  auto iter = links_.find(link);
  if (iter == links_.end()) {
    return MediaResult::NOT_FOUND;
  }

  links_.erase(iter);
  return MediaResult::OK;
}

MediaResult AudioOutput::Init() {
  return MediaResult::OK;
}

void AudioOutput::Cleanup() {
}

MediaResult AudioOutput::InitializeLink(const AudioTrackToOutputLinkPtr& link) {
  DCHECK(link);
  return MediaResult::OK;
}

void AudioOutput::ScheduleCallback(LocalTime when) {
  base::AutoLock lock(shutdown_lock_);

  // If we are in the process of shutting down, then we are no longer permitted
  // to schedule callbacks.
  if (shutting_down_) {
    DCHECK(!task_runner_);
    return;
  }
  DCHECK(task_runner_);

  // TODO(johngro):  Someday, if there is ever a way to schedule delayed tasks
  // with absolute time, or with resolution better than microseconds, do so.
  // Until then figure out the relative time for scheduling the task and do so.
  LocalTime now = LocalClock::now();
  base::TimeDelta sched_time = (now > when)
    ? base::TimeDelta::FromMicroseconds(0)
    : base::TimeDelta::FromMicroseconds(
        local_time::to_usec<int64_t>(when - now));

  task_runner_->PostNonNestableDelayedTask(
      FROM_HERE,
      base::Bind(&ProcessThunk, weak_self_),
      sched_time);
}

void AudioOutput::ShutdownSelf() {
  // If we are not already in the process of shutting down, send a message to
  // the main message loop telling it to complete the shutdown process.
  if (!BeginShutdown()) {
    DCHECK(manager_);
    manager_->ScheduleMessageLoopTask(
        FROM_HERE,
        base::Bind(&FinishShutdownSelf, manager_, weak_self_));
  }
}

void AudioOutput::ProcessThunk(AudioOutputWeakPtr weak_output) {
  // If we are still around by the time this callback fires, enter the procesing
  // lock and dispatch to our derived class's implementation.
  auto output  = weak_output.lock();
  if (output) {
    base::AutoLock lock(output->processing_lock_);

    // Make sure that we are not in the process of cleaning up before we start
    // processing.
    if (!output->shutting_down_) {
      output->Process();
    }
  }
}

MediaResult AudioOutput::Init(
    const AudioOutputPtr& self,
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  DCHECK(this == self.get());
  DCHECK(task_runner);

  // If our derived class failed to initialize, don't bother to hold onto the
  // state we will need drive our callback engine.  Begin the process of
  // shutting ourselves down, the output manager will eventually finish the job
  // for us.
  MediaResult res = Init();
  if (res != MediaResult::OK) {
    ShutdownSelf();
    return res;
  }

  // Stash our callback state and schedule an immediate callback to get things
  // running.
  task_runner_ = task_runner;
  weak_self_   = self;
  task_runner_->PostNonNestableTask(FROM_HERE,
                                    base::Bind(&ProcessThunk, weak_self_));

  return MediaResult::OK;
}

bool AudioOutput::BeginShutdown() {
  // Start the process of shutting down if we have not already.  This method may
  // be called from either a processing context, or from the audio output
  // manager.  After it finishes, any pending processing callbacks will have
  // been nerfed, although there may still be callbacks in flight.
  base::AutoLock lock(shutdown_lock_);

  if (shutting_down_) { return true; }

  shutting_down_ = true;
  task_runner_   = nullptr;

  return false;
}

void AudioOutput::Shutdown() {
  if (shut_down_) { return; }

  // TODO(johngro): Assert that we are running on the audio server's main
  // message loop thread.

  // Make sure no new callbacks can be generated, and that pending callbacks
  // have been nerfed.
  BeginShutdown();

  // Synchronize with any callbacks in flight.  By acquiring and releasing the
  // processing lock, we are guaranteed that we have no callbacks which are in
  // the middle of processing, and that any pending callbacks will be nerfed.
  // It is safe to destroy this audio output at any point in time after this.
  processing_lock_.Acquire();
  processing_lock_.Release();

  // Unlink ourselves from all of our tracks.  Then go ahead and clear the track
  // set.
  for (const auto& link : links_) {
    DCHECK(link);
    AudioTrackImplPtr track = link->GetTrack();
    if (track) {
      track->RemoveOutput(link);
    }
  }
  links_.clear();

  // Give our derived class a chance to clean up its resources.
  Cleanup();

  // We are now completely shut down.  The only reason we have this flag is to
  // make sure that Shutdown is idempotent.
  shut_down_ = true;
}

}  // namespace audio
}  // namespace media
}  // namespace mojo

