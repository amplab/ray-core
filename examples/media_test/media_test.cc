// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/media_test/media_test.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/services/media/audio/interfaces/audio_server.mojom.h"
#include "mojo/services/media/audio/interfaces/audio_track.mojom.h"
#include "mojo/services/media/common/cpp/timeline.h"
#include "mojo/services/media/common/cpp/timeline_function.h"
#include "mojo/services/media/control/interfaces/media_factory.mojom.h"

namespace mojo {
namespace media {
namespace examples {

// static
std::unique_ptr<MediaTest> MediaTest::Create(
    mojo::Shell* shell,
    const std::string& input_file_name) {
  return std::unique_ptr<MediaTest>(new MediaTest(shell, input_file_name));
}

MediaTest::MediaTest(mojo::Shell* shell, const std::string& input_file_name) {
  MediaFactoryPtr factory;
  ConnectToService(shell, "mojo:media_factory", GetProxy(&factory));

  AudioServerPtr audio_service;
  ConnectToService(shell, "mojo:audio_server", GetProxy(&audio_service));
  AudioTrackPtr audio_track;
  MediaRendererPtr audio_renderer;
  audio_service->CreateTrack(GetProxy(&audio_track), GetProxy(&audio_renderer));

  SeekingReaderPtr reader;
  factory->CreateNetworkReader(input_file_name, GetProxy(&reader));

  factory->CreatePlayer(reader.Pass(), audio_renderer.Pass(), nullptr,
                        GetProxy(&media_player_));

  HandleStatusUpdates();
}

MediaTest::~MediaTest() {}

int64_t MediaTest::position_ns() const {
  // Apply the timeline function to the current time.
  int64_t position = timeline_function_(Timeline::local_now());

  if (position < 0) {
    position = 0;
  }

  if (metadata_ && static_cast<uint64_t>(position) > metadata_->duration) {
    position = metadata_->duration;
  }

  return position;
}

const MediaMetadataPtr& MediaTest::metadata() const {
  return metadata_;
}

void MediaTest::HandleStatusUpdates(uint64_t version,
                                    MediaPlayerStatusPtr status) {
  if (status) {
    // Process status received from the player.
    if (status->timeline_transform) {
      timeline_function_ = status->timeline_transform.To<TimelineFunction>();
    }

    previous_state_ = state_;
    if (status->end_of_stream) {
      state_ = State::kEnded;
    } else if (timeline_function_.subject_delta() == 0) {
      state_ = State::kPaused;
    } else {
      state_ = State::kPlaying;
    }

    metadata_ = status->metadata.Pass();

    if (update_callback_ != nullptr) {
      update_callback_();
    }
  }

  // Request a status update.
  media_player_->GetStatus(
      version, [this](uint64_t version, MediaPlayerStatusPtr status) {
        HandleStatusUpdates(version, status.Pass());
      });
}

}  // namespace examples
}  // namespace media
}  // namespace mojo
