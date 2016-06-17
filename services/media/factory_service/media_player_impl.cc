// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/services/media/common/cpp/timeline.h"
#include "services/media/factory_service/media_player_impl.h"
#include "services/media/framework/parts/reader.h"
#include "services/media/framework/util/callback_joiner.h"

namespace mojo {
namespace media {

// static
std::shared_ptr<MediaPlayerImpl> MediaPlayerImpl::Create(
    InterfaceHandle<SeekingReader> reader,
    InterfaceHandle<MediaRenderer> audio_renderer,
    InterfaceHandle<MediaRenderer> video_renderer,
    InterfaceRequest<MediaPlayer> request,
    MediaFactoryService* owner) {
  return std::shared_ptr<MediaPlayerImpl>(
      new MediaPlayerImpl(reader.Pass(), audio_renderer.Pass(),
                          video_renderer.Pass(), request.Pass(), owner));
}

MediaPlayerImpl::MediaPlayerImpl(InterfaceHandle<SeekingReader> reader,
                                 InterfaceHandle<MediaRenderer> audio_renderer,
                                 InterfaceHandle<MediaRenderer> video_renderer,
                                 InterfaceRequest<MediaPlayer> request,
                                 MediaFactoryService* owner)
    : MediaFactoryService::Product<MediaPlayer>(this, request.Pass(), owner) {
  DCHECK(reader);

  status_publisher_.SetCallbackRunner([this](const GetStatusCallback& callback,
                                             uint64_t version) {
    MediaPlayerStatusPtr status = MediaPlayerStatus::New();
    status->timeline_transform = TimelineTransform::From(timeline_function_);
    status->end_of_stream = end_of_stream_;
    status->metadata = metadata_.Clone();
    callback.Run(version, status.Pass());
  });

  state_ = State::kWaiting;

  ConnectToService(owner->shell(), "mojo:media_factory", GetProxy(&factory_));

  factory_->CreateDemux(reader.Pass(), GetProxy(&demux_));
  HandleDemuxMetadataUpdates();

  factory_->CreateTimelineController(GetProxy(&timeline_controller_));
  timeline_controller_->GetControlSite(GetProxy(&timeline_control_site_));
  timeline_control_site_->GetTimelineConsumer(GetProxy(&timeline_consumer_));
  HandleTimelineControlSiteStatusUpdates();

  audio_renderer_ = audio_renderer.Pass();
  video_renderer_ = video_renderer.Pass();

  demux_->Describe([this](mojo::Array<MediaTypePtr> stream_types) {
    // Populate streams_ and enable the streams we want.
    std::shared_ptr<CallbackJoiner> callback_joiner = CallbackJoiner::Create();

    for (MediaTypePtr& stream_type : stream_types) {
      streams_.push_back(std::unique_ptr<Stream>(new Stream()));
      Stream& stream = *streams_.back();
      switch (stream_type->medium) {
        case MediaTypeMedium::AUDIO:
          stream.renderer_ = audio_renderer_.Pass();
          PrepareStream(&stream, streams_.size() - 1, stream_type,
                        callback_joiner->NewCallback());
          break;
        case MediaTypeMedium::VIDEO:
          stream.renderer_ = video_renderer_.Pass();
          PrepareStream(&stream, streams_.size() - 1, stream_type,
                        callback_joiner->NewCallback());
          break;
        // TODO(dalesat): Enable other stream types.
        default:
          break;
      }
    }

    callback_joiner->WhenJoined([this]() {
      // The enabled streams are prepared.
      factory_.reset();
      state_ = State::kPaused;
      Update();
    });
  });
}

MediaPlayerImpl::~MediaPlayerImpl() {}

void MediaPlayerImpl::Update() {
  while (true) {
    switch (state_) {
      case State::kPaused:
        if (target_position_ != kUnspecifiedTime) {
          WhenPausedAndSeeking();
          break;
        }

        if (target_state_ == State::kPlaying) {
          if (!flushed_) {
            SetTimelineTransform(1, 1);
            state_ = State::kWaiting;
            break;
          }

          flushed_ = false;
          state_ = State::kWaiting;
          demux_->Prime([this]() {
            SetTimelineTransform(1, 1);
            state_ = State::kWaiting;
            Update();
          });
        }
        return;

      case State::kPlaying:
        if (target_position_ != kUnspecifiedTime ||
            target_state_ == State::kPaused) {
          SetTimelineTransform(1, 0);
          state_ = State::kWaiting;
          break;
        }

        if (end_of_stream_) {
          target_state_ = State::kPaused;
          state_ = State::kPaused;
          break;
        }
        return;

      case State::kWaiting:
        return;
    }
  }
}

void MediaPlayerImpl::WhenPausedAndSeeking() {
  if (!flushed_) {
    state_ = State::kWaiting;
    demux_->Flush([this]() {
      flushed_ = true;
      WhenFlushedAndSeeking();
    });
  } else {
    WhenFlushedAndSeeking();
  }
}

void MediaPlayerImpl::WhenFlushedAndSeeking() {
  state_ = State::kWaiting;
  DCHECK(target_position_ != kUnspecifiedTime);
  demux_->Seek(target_position_, [this]() {
    transform_subject_time_ = target_position_;
    target_position_ = kUnspecifiedTime;
    state_ = State::kPaused;
    Update();
  });
}

void MediaPlayerImpl::SetTimelineTransform(uint32_t reference_delta,
                                           uint32_t subject_delta) {
  timeline_consumer_->SetTimelineTransform(
      transform_subject_time_, reference_delta, subject_delta,
      Timeline::local_now() + kMinimumLeadTime, kUnspecifiedTime,
      [this, subject_delta](bool completed) {
        RCHECK(state_ == State::kWaiting);

        if (subject_delta == 0) {
          state_ = State::kPaused;
        } else {
          state_ = State::kPlaying;
        }

        Update();
      });
}

void MediaPlayerImpl::GetStatus(uint64_t version_last_seen,
                                const GetStatusCallback& callback) {
  status_publisher_.Get(version_last_seen, callback);
}

void MediaPlayerImpl::Play() {
  target_state_ = State::kPlaying;
  Update();
}

void MediaPlayerImpl::Pause() {
  target_state_ = State::kPaused;
  Update();
}

void MediaPlayerImpl::Seek(int64_t position) {
  target_position_ = position;
  Update();
}

void MediaPlayerImpl::PrepareStream(Stream* stream,
                                    size_t index,
                                    const MediaTypePtr& input_media_type,
                                    const std::function<void()>& callback) {
  DCHECK(factory_);

  demux_->GetProducer(index, GetProxy(&stream->encoded_producer_));

  if (input_media_type->encoding != MediaType::kAudioEncodingLpcm &&
      input_media_type->encoding != MediaType::kVideoEncodingUncompressed) {
    std::shared_ptr<CallbackJoiner> callback_joiner = CallbackJoiner::Create();

    // Compressed media. Insert a decoder in front of the sink. The sink would
    // add its own internal decoder, but we want to test the decoder.
    factory_->CreateDecoder(input_media_type.Clone(),
                            GetProxy(&stream->decoder_));

    MediaConsumerPtr decoder_consumer;
    stream->decoder_->GetConsumer(GetProxy(&decoder_consumer));

    callback_joiner->Spawn();
    stream->encoded_producer_->Connect(decoder_consumer.Pass(),
                                       [stream, callback_joiner]() {
                                         stream->encoded_producer_.reset();
                                         callback_joiner->Complete();
                                       });

    callback_joiner->Spawn();
    stream->decoder_->GetOutputType(
        [this, stream, callback_joiner](MediaTypePtr output_type) {
          stream->decoder_->GetProducer(GetProxy(&stream->decoded_producer_));
          CreateSink(stream, output_type, callback_joiner->NewCallback());
          callback_joiner->Complete();
        });

    callback_joiner->WhenJoined(callback);
  } else {
    // Uncompressed media. Connect the demux stream directly to the sink. This
    // would work for compressed media as well (the sink would decode), but we
    // want to test the decoder.
    stream->decoded_producer_ = stream->encoded_producer_.Pass();
    CreateSink(stream, input_media_type, callback);
  }
}

void MediaPlayerImpl::CreateSink(Stream* stream,
                                 const MediaTypePtr& input_media_type,
                                 const std::function<void()>& callback) {
  DCHECK(input_media_type);
  DCHECK(stream->decoded_producer_);
  DCHECK(factory_);

  factory_->CreateSink(stream->renderer_.Pass(), input_media_type.Clone(),
                       GetProxy(&stream->sink_));

  MediaTimelineControlSitePtr timeline_control_site;
  stream->sink_->GetTimelineControlSite(GetProxy(&timeline_control_site));

  timeline_controller_->AddControlSite(timeline_control_site.Pass());

  MediaConsumerPtr consumer;
  stream->sink_->GetConsumer(GetProxy(&consumer));

  stream->decoded_producer_->Connect(consumer.Pass(),
                                     [this, callback, stream]() {
                                       stream->decoded_producer_.reset();
                                       callback();
                                     });
}

void MediaPlayerImpl::HandleDemuxMetadataUpdates(uint64_t version,
                                                 MediaMetadataPtr metadata) {
  if (metadata) {
    metadata_ = metadata.Pass();
    status_publisher_.SendUpdates();
  }

  demux_->GetMetadata(version,
                      [this](uint64_t version, MediaMetadataPtr metadata) {
                        HandleDemuxMetadataUpdates(version, metadata.Pass());
                      });
}

void MediaPlayerImpl::HandleTimelineControlSiteStatusUpdates(
    uint64_t version,
    MediaTimelineControlSiteStatusPtr status) {
  if (status) {
    timeline_function_ = status->timeline_transform.To<TimelineFunction>();
    end_of_stream_ = status->end_of_stream;
    status_publisher_.SendUpdates();
    Update();
  }

  timeline_control_site_->GetStatus(
      version,
      [this](uint64_t version, MediaTimelineControlSiteStatusPtr status) {
        HandleTimelineControlSiteStatusUpdates(version, status.Pass());
      });
}

MediaPlayerImpl::Stream::Stream() {}

MediaPlayerImpl::Stream::~Stream() {}

}  // namespace media
}  // namespace mojo
