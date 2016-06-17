// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_MEDIA_FACTORY_MEDIA_PLAYER_IMPL_H_
#define MOJO_SERVICES_MEDIA_FACTORY_MEDIA_PLAYER_IMPL_H_

#include <limits>
#include <vector>

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/media/common/cpp/timeline.h"
#include "mojo/services/media/common/cpp/timeline_function.h"
#include "mojo/services/media/common/interfaces/media_transport.mojom.h"
#include "mojo/services/media/control/interfaces/media_factory.mojom.h"
#include "mojo/services/media/core/interfaces/seeking_reader.mojom.h"
#include "mojo/services/media/core/interfaces/timeline_controller.mojom.h"
#include "services/media/common/mojo_publisher.h"
#include "services/media/factory_service/factory_service.h"
#include "services/media/framework/util/callback_joiner.h"

namespace mojo {
namespace media {

// Mojo agent that renders streams from an origin specified by URL.
class MediaPlayerImpl : public MediaFactoryService::Product<MediaPlayer>,
                        public MediaPlayer {
 public:
  static std::shared_ptr<MediaPlayerImpl> Create(
      InterfaceHandle<SeekingReader> reader,
      InterfaceHandle<MediaRenderer> audio_renderer,
      InterfaceHandle<MediaRenderer> video_renderer,
      InterfaceRequest<MediaPlayer> request,
      MediaFactoryService* owner);

  ~MediaPlayerImpl() override;

  // MediaPlayer implementation.
  void GetStatus(uint64_t version_last_seen,
                 const GetStatusCallback& callback) override;

  void Play() override;

  void Pause() override;

  void Seek(int64_t position) override;

 private:
  static constexpr int64_t kMinimumLeadTime = Timeline::ns_from_ms(30);

  // Internal state.
  enum class State {
    kWaiting,  // Waiting for some work to complete.
    kPaused,
    kPlaying,
  };

  struct Stream {
    Stream();
    ~Stream();
    // TODO(dalesat): Have the sink enlist the decoder.
    MediaTypeConverterPtr decoder_;
    MediaSinkPtr sink_;
    // The following fields are just temporaries used to solve lambda capture
    // problems.
    MediaProducerPtr encoded_producer_;
    MediaProducerPtr decoded_producer_;
    InterfaceHandle<MediaRenderer> renderer_;
  };

  MediaPlayerImpl(InterfaceHandle<SeekingReader> reader,
                  InterfaceHandle<MediaRenderer> audio_renderer,
                  InterfaceHandle<MediaRenderer> video_renderer,
                  InterfaceRequest<MediaPlayer> request,
                  MediaFactoryService* owner);

  // Takes action based on current state.
  void Update();

  // Handles seeking in paused state.
  void WhenPausedAndSeeking();

  // Handles seeking in paused state with flushed pipeline.
  void WhenFlushedAndSeeking();

  // Sets the timeline transform. transform_subject_time_ is used for the
  // subject_time, and the effective_reference_time is now plus
  // kMinimumLeadTime.
  void SetTimelineTransform(uint32_t reference_delta, uint32_t subject_delta);

  // Prepares a stream.
  void PrepareStream(Stream* stream,
                     size_t index,
                     const MediaTypePtr& input_media_type,
                     const std::function<void()>& callback);

  // Creates a sink for a stream.
  void CreateSink(Stream* stream,
                  const MediaTypePtr& input_media_type,
                  const std::function<void()>& callback);

  // Handles a metadata update from the demux. When called with the default
  // argument values, initiates demux metadata updates.
  void HandleDemuxMetadataUpdates(
      uint64_t version = MediaDemux::kInitialMetadata,
      MediaMetadataPtr metadata = nullptr);

  // Handles a status update from the control site. When called with the default
  // argument values, initiates control site. status updates.
  void HandleTimelineControlSiteStatusUpdates(
      uint64_t version = MediaTimelineControlSite::kInitialStatus,
      MediaTimelineControlSiteStatusPtr status = nullptr);

  MediaFactoryPtr factory_;
  MediaDemuxPtr demux_;
  MediaTimelineControllerPtr timeline_controller_;
  MediaTimelineControlSitePtr timeline_control_site_;
  TimelineConsumerPtr timeline_consumer_;
  std::vector<std::unique_ptr<Stream>> streams_;
  State state_ = State::kWaiting;
  State target_state_ = State::kPaused;
  bool flushed_ = true;
  bool end_of_stream_ = false;
  int64_t target_position_ = kUnspecifiedTime;
  int64_t transform_subject_time_ = kUnspecifiedTime;
  TimelineFunction timeline_function_;
  CallbackJoiner set_transform_joiner_;
  MediaMetadataPtr metadata_;
  MojoPublisher<GetStatusCallback> status_publisher_;
  // The following fields are just temporaries used to solve lambda capture
  // problems.
  InterfaceHandle<MediaRenderer> audio_renderer_;
  InterfaceHandle<MediaRenderer> video_renderer_;
};

}  // namespace media
}  // namespace mojo

#endif  // MOJO_SERVICES_MEDIA_FACTORY_MEDIA_PLAYER_IMPL_H_
