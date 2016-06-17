// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_MEDIA_FACTORY_MEDIA_SINK_IMPL_H_
#define MOJO_SERVICES_MEDIA_FACTORY_MEDIA_SINK_IMPL_H_

#include <memory>

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/media/control/interfaces/media_sink.mojom.h"
#include "mojo/services/media/core/interfaces/timeline_controller.mojom.h"
#include "services/media/factory_service/factory_service.h"
#include "services/media/framework/graph.h"
#include "services/media/framework/parts/decoder.h"
#include "services/media/framework_mojo/mojo_consumer.h"
#include "services/media/framework_mojo/mojo_producer.h"
#include "services/util/cpp/incident.h"

namespace mojo {
namespace media {

// Mojo agent that consumes a stream and delivers it to a destination specified
// by URL.
class MediaSinkImpl : public MediaFactoryService::Product<MediaSink>,
                      public MediaSink {
 public:
  static std::shared_ptr<MediaSinkImpl> Create(
      InterfaceHandle<MediaRenderer> renderer,
      MediaTypePtr media_type,
      InterfaceRequest<MediaSink> request,
      MediaFactoryService* owner);

  ~MediaSinkImpl() override;

  // MediaSink implementation.
  void GetConsumer(InterfaceRequest<MediaConsumer> consumer) override;

  void GetTimelineControlSite(
      InterfaceRequest<MediaTimelineControlSite> req) override;

 private:
  class NullTimelineControlSite : public MediaTimelineControlSite,
                                  public TimelineConsumer {
   public:
    NullTimelineControlSite(
        InterfaceRequest<MediaTimelineControlSite> control_site_request);

    ~NullTimelineControlSite() override;

    // MediaTimelineControlSite implementation.
    void GetStatus(uint64_t version_last_seen,
                   const GetStatusCallback& callback) override;

    void GetTimelineConsumer(
        InterfaceRequest<TimelineConsumer> timeline_consumer) override;

    // TimelineConsumer implementation.
    void SetTimelineTransform(
        int64_t subject_time,
        uint32_t reference_delta,
        uint32_t subject_delta,
        int64_t effective_reference_time,
        int64_t effective_subject_time,
        const SetTimelineTransformCallback& callback) override;

   private:
    Binding<MediaTimelineControlSite> control_site_binding_;
    Binding<TimelineConsumer> consumer_binding_;
    GetStatusCallback get_status_callback_;
  };

  MediaSinkImpl(InterfaceHandle<MediaRenderer> renderer,
                MediaTypePtr media_type,
                InterfaceRequest<MediaSink> request,
                MediaFactoryService* owner);

  Incident ready_;
  Graph graph_;
  std::shared_ptr<MojoConsumer> consumer_;
  std::shared_ptr<MojoProducer> producer_;
  MediaRendererPtr renderer_;
  // The following fields are just temporaries used to solve lambda capture
  // problems.
  std::unique_ptr<StreamType> input_stream_type_;
};

}  // namespace media
}  // namespace mojo

#endif  // MOJO_SERVICES_MEDIA_FACTORY_MEDIA_SINK_IMPL_H_
