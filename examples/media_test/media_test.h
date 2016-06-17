// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_MEDIA_TEST_MEDIA_TEST_H_
#define EXAMPLES_MEDIA_TEST_MEDIA_TEST_H_

#include "base/macros.h"
#include "mojo/services/media/common/cpp/timeline_function.h"
#include "mojo/services/media/control/interfaces/media_factory.mojom.h"
#include "mojo/services/media/control/interfaces/media_player.mojom.h"

namespace mojo {

class Shell;

namespace media {
namespace examples {

// Model for media test application.
class MediaTest {
 public:
  enum class State { kPaused, kPlaying, kEnded };

  using UpdateCallback = std::function<void()>;

  static std::unique_ptr<MediaTest> Create(mojo::Shell* shell,
                                           const std::string& input_file_name);

  ~MediaTest();

  // Registers a callback signalling that the app should update its view.
  void RegisterUpdateCallback(const UpdateCallback& callback) {
    update_callback_ = callback;
  }

  // Starts playback.
  void Play() { media_player_->Play(); }

  // Pauses playback.
  void Pause() { media_player_->Pause(); }

  // Seeks to the position indicated in nanoseconds from the start of the media.
  void Seek(int64_t position_ns) { media_player_->Seek(position_ns); }

  // Returns the previous state of the player.
  State previous_state() const { return previous_state_; }

  // Returns the current state of the player.
  State state() const { return state_; }

  // Returns the current presentation time in nanoseconds.
  int64_t position_ns() const;

  // Returns the current media metadata, if there is any.
  const MediaMetadataPtr& metadata() const;

 private:
  MediaTest(mojo::Shell* shell, const std::string& input_file_name);

  // Handles a status update from the player. When called with the default
  // argument values, initiates status updates.
  void HandleStatusUpdates(uint64_t version = MediaPlayer::kInitialStatus,
                           MediaPlayerStatusPtr status = nullptr);

  MediaPlayerPtr media_player_;
  State previous_state_ = State::kPaused;
  State state_ = State::kPaused;
  TimelineFunction timeline_function_;
  MediaMetadataPtr metadata_;
  UpdateCallback update_callback_;

  DISALLOW_COPY_AND_ASSIGN(MediaTest);
};

}  // namespace examples
}  // namespace media
}  // namespace mojo

#endif  // EXAMPLES_MEDIA_TEST_MEDIA_TEST_H_
