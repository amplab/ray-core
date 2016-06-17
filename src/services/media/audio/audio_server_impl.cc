// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"
#include "base/message_loop/message_loop.h"
#include "base/task_runner.h"

#include "services/media/audio/audio_output_manager.h"
#include "services/media/audio/audio_server_impl.h"
#include "services/media/audio/audio_track_impl.h"

namespace mojo {
namespace media {
namespace audio {

AudioServerImpl::AudioServerImpl()
  : output_manager_(this),
    cleanup_queue_(new CleanupQueue) {
  cleanup_closure_ = base::Bind(
      &AudioServerImpl::DoPacketCleanup,
      base::Unretained(this));
}

AudioServerImpl::~AudioServerImpl() {
  Shutdown();
  DCHECK(cleanup_queue_);
  DCHECK_EQ(cleanup_queue_->size(), 0u);
}

void AudioServerImpl::Initialize() {
  // Stash a pointer to our task runner.
  DCHECK(base::MessageLoop::current());
  task_runner_ = base::MessageLoop::current()->task_runner();
  DCHECK(task_runner_);

  // Set up our output manager.
  MediaResult res = output_manager_.Init();
  // TODO(johngro): Do better at error handling than this weak check.
  DCHECK(res == MediaResult::OK);
}

void AudioServerImpl::Shutdown() {
  shutting_down_ = true;

  while (tracks_.size()) {
    // Tracks remove themselves from the server's set of active tracks as they
    // shutdown.  Assert that the set's size is shrinking by one each time we
    // shut down a track so we know that we are making progress.
    size_t size_before = tracks_.size();
    (*tracks_.begin())->Shutdown();
    size_t size_after = tracks_.size();
    DCHECK_LT(size_after, size_before);
  }

  output_manager_.Shutdown();
  DoPacketCleanup();
}

void AudioServerImpl::CreateTrack(InterfaceRequest<AudioTrack> track,
    InterfaceRequest<MediaRenderer> renderer) {
  tracks_.insert(AudioTrackImpl::Create(track.Pass(), renderer.Pass(), this));
}

void AudioServerImpl::DoPacketCleanup() {
  // In order to minimize the time we spend in the lock, we allocate a new
  // queue, then lock, swap and clear the sched flag, and finally clean out the
  // queue (which has the side effect of triggering all of the send packet
  // callbacks).
  //
  // Note: this is only safe because we know that we are executing on a single
  // threaded task runner.  Without this guarantee, it might be possible call
  // the send packet callbacks for a media pipe in a different order than the
  // packets were sent in the first place.  If the task_runner for the audio
  // server ever loses this serialization guarantee (because it becomes
  // multi-threaded, for example) we will need to introduce another lock
  // (different from the cleanup lock) in order to keep the cleanup tasks
  // properly ordered while guaranteeing minimal contention of the cleanup lock
  // (which is being acquired by the high priority mixing threads).
  std::unique_ptr<CleanupQueue> tmp_queue(new CleanupQueue());

  {
    base::AutoLock lock(cleanup_queue_lock_);
    cleanup_queue_.swap(tmp_queue);
    cleanup_scheduled_ = false;
  }

  // The clear method of standard containers do not guarantee any ordering of
  // destruction of the objects they hold.  In order to guarantee proper
  // sequencing of the callbacks, go over the container front-to-back, nulling
  // out the std::unique_ptrs they hold as we go (which will trigger the
  // callbacks).  Afterwards, just let tmp_queue go out of scope and clear()
  // itself automatically.
  for (auto iter = tmp_queue->begin(); iter != tmp_queue->end(); ++iter) {
    (*iter) = nullptr;
  }
}

void AudioServerImpl::SchedulePacketCleanup(
    MediaPipeBase::MediaPacketStatePtr state) {
  base::AutoLock lock(cleanup_queue_lock_);

  cleanup_queue_->emplace_back(std::move(state));

  if (!cleanup_scheduled_ && !shutting_down_) {
    DCHECK(task_runner_);
    cleanup_scheduled_ = task_runner_->PostTask(FROM_HERE, cleanup_closure_);
    DCHECK(cleanup_scheduled_);
  }
}

}  // namespace audio
}  // namespace media
}  // namespace mojo
