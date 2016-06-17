// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "services/media/factory_service/media_sink_impl.h"
#include "services/media/framework/util/conversion_pipeline_builder.h"
#include "services/media/framework_mojo/mojo_type_conversions.h"

namespace mojo {
namespace media {

// static
std::shared_ptr<MediaSinkImpl> MediaSinkImpl::Create(
    InterfaceHandle<MediaRenderer> renderer,
    MediaTypePtr media_type,
    InterfaceRequest<MediaSink> request,
    MediaFactoryService* owner) {
  return std::shared_ptr<MediaSinkImpl>(new MediaSinkImpl(
      renderer.Pass(), media_type.Pass(), request.Pass(), owner));
}

MediaSinkImpl::MediaSinkImpl(InterfaceHandle<MediaRenderer> renderer,
                             MediaTypePtr media_type,
                             InterfaceRequest<MediaSink> request,
                             MediaFactoryService* owner)
    : MediaFactoryService::Product<MediaSink>(this, request.Pass(), owner),
      consumer_(MojoConsumer::Create()),
      producer_(MojoProducer::Create()),
      renderer_(MediaRendererPtr::Create(renderer.Pass())) {
  DCHECK(media_type);

  PartRef consumer_ref = graph_.Add(consumer_);
  PartRef producer_ref = graph_.Add(producer_);

  consumer_->SetPrimeRequestedCallback(
      [this](const MediaConsumer::PrimeCallback& callback) {
        ready_.When([this, callback]() {
          DCHECK(producer_);
          producer_->PrimeConnection(callback);
        });
      });
  consumer_->SetFlushRequestedCallback(
      [this, consumer_ref](const MediaConsumer::FlushCallback& callback) {
        ready_.When([this, consumer_ref, callback]() {
          DCHECK(producer_);
          graph_.FlushOutput(consumer_ref.output());
          producer_->FlushConnection(callback);
        });
      });

  // TODO(dalesat): Temporary, remove.
  if (!renderer_) {
    // Throwing away the content.
    graph_.ConnectParts(consumer_ref, producer_ref);
    graph_.Prepare();
    ready_.Occur();
    return;
  }

  // TODO(dalesat): Once we have c++14, get rid of this shared pointer hack.
  input_stream_type_ = media_type.To<std::unique_ptr<StreamType>>();

  renderer_->GetSupportedMediaTypes([this, consumer_ref, producer_ref](
      Array<MediaTypeSetPtr> supported_media_types) {
    std::unique_ptr<std::vector<std::unique_ptr<StreamTypeSet>>>
        supported_stream_types = supported_media_types.To<std::unique_ptr<
            std::vector<std::unique_ptr<media::StreamTypeSet>>>>();
    std::unique_ptr<StreamType> producer_stream_type;

    // Add transforms to the pipeline to convert from stream_type to a
    // type supported by the track.
    OutputRef out = consumer_ref.output();
    bool result =
        BuildConversionPipeline(*input_stream_type_, *supported_stream_types,
                                &graph_, &out, &producer_stream_type);
    if (!result) {
      // Failed to build conversion pipeline.
      LOG(WARNING) << "failed to build conversion pipeline";
      // TODO(dalesat): Add problem reporting.
      return;
    }

    graph_.ConnectOutputToPart(out, producer_ref);

    renderer_->SetMediaType(MediaType::From(std::move(producer_stream_type)));
    MediaConsumerPtr consumer;
    renderer_->GetConsumer(GetProxy(&consumer));
    producer_->Connect(consumer.Pass(), [this]() {
      graph_.Prepare();
      ready_.Occur();
    });
  });
}

MediaSinkImpl::~MediaSinkImpl() {}

void MediaSinkImpl::GetConsumer(InterfaceRequest<MediaConsumer> consumer) {
  consumer_->AddBinding(consumer.Pass());
}

void MediaSinkImpl::GetTimelineControlSite(
    InterfaceRequest<MediaTimelineControlSite> req) {
  if (renderer_) {
    renderer_->GetTimelineControlSite(req.Pass());
    return;
  }

  new NullTimelineControlSite(req.Pass());
}

MediaSinkImpl::NullTimelineControlSite::NullTimelineControlSite(
    InterfaceRequest<MediaTimelineControlSite> control_site_request)
    : control_site_binding_(this, control_site_request.Pass()),
      consumer_binding_(this) {}

MediaSinkImpl::NullTimelineControlSite::~NullTimelineControlSite() {}

void MediaSinkImpl::NullTimelineControlSite::GetStatus(
    uint64_t version_last_seen,
    const GetStatusCallback& callback) {
  DCHECK(get_status_callback_.is_null());
  get_status_callback_ = callback;
}

void MediaSinkImpl::NullTimelineControlSite::GetTimelineConsumer(
    InterfaceRequest<TimelineConsumer> timeline_consumer) {
  consumer_binding_.Bind(timeline_consumer.Pass());
}

void MediaSinkImpl::NullTimelineControlSite::SetTimelineTransform(
    int64_t subject_time,
    uint32_t reference_delta,
    uint32_t subject_delta,
    int64_t effective_reference_time,
    int64_t effective_subject_time,
    const SetTimelineTransformCallback& callback) {
  callback.Run(true);
}

}  // namespace media
}  // namespace mojo
