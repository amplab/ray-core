// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_MEDIA_FACTORY_MEDIA_SOURCE_IMPL_H_
#define MOJO_SERVICES_MEDIA_FACTORY_MEDIA_SOURCE_IMPL_H_

#include <vector>

#include "base/single_thread_task_runner.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/media/control/interfaces/media_source.mojom.h"
#include "mojo/services/media/core/interfaces/seeking_reader.mojom.h"
#include "services/media/common/mojo_publisher.h"
#include "services/media/factory_service/factory_service.h"
#include "services/media/framework/graph.h"
#include "services/media/framework/parts/decoder.h"
#include "services/media/framework/parts/demux.h"
#include "services/media/framework/parts/null_sink.h"
#include "services/media/framework/parts/reader.h"
#include "services/media/framework_mojo/mojo_producer.h"
#include "services/media/framework_mojo/mojo_pull_mode_producer.h"
#include "services/util/cpp/incident.h"

namespace mojo {
namespace media {

// Mojo agent that produces streams from an origin specified by URL.
class MediaSourceImpl : public MediaFactoryService::Product<MediaSource>,
                        public MediaSource {
 public:
  static std::shared_ptr<MediaSourceImpl> Create(
      InterfaceHandle<SeekingReader> reader,
      const Array<MediaTypeSetPtr>& allowed_media_types,
      InterfaceRequest<MediaSource> request,
      MediaFactoryService* owner);

  ~MediaSourceImpl() override;

  // MediaSource implementation.
  void GetStreams(const GetStreamsCallback& callback) override;

  void GetProducer(uint32_t stream_index,
                   InterfaceRequest<MediaProducer> producer) override;

  void GetPullModeProducer(
      uint32_t stream_index,
      InterfaceRequest<MediaPullModeProducer> producer) override;

  void GetStatus(uint64_t version_last_seen,
                 const GetStatusCallback& callback) override;

  void Prepare(const PrepareCallback& callback) override;

  void Prime(const PrimeCallback& callback) override;

  void Flush(const FlushCallback& callback) override;

  void Seek(int64_t position, const SeekCallback& callback) override;

 private:
  MediaSourceImpl(InterfaceHandle<SeekingReader> reader,
                  const Array<MediaTypeSetPtr>& allowed_media_types,
                  InterfaceRequest<MediaSource> request,
                  MediaFactoryService* owner);

  class Stream {
   public:
    Stream(OutputRef output,
           std::unique_ptr<StreamType> stream_type,
           const std::unique_ptr<std::vector<std::unique_ptr<StreamTypeSet>>>&
               allowed_stream_types,
           Graph* graph);

    ~Stream();

    // Gets the media type of the stream.
    MediaTypePtr media_type() const;

    // Gets the original stream type of the stream.
    MediaTypePtr original_media_type() const;

    // Gets the producer.
    void GetProducer(InterfaceRequest<MediaProducer> producer);

    // Gets the pull mode producer.
    void GetPullModeProducer(InterfaceRequest<MediaPullModeProducer> producer);

    // Makes sure the stream has a sink.
    void EnsureSink();

    // Tells the producer to prime its connection.
    void PrimeConnection(const MojoProducer::PrimeConnectionCallback callback);

    // Tells the producer to flush its connection.
    void FlushConnection(const MojoProducer::FlushConnectionCallback callback);

   private:
    std::unique_ptr<StreamType> stream_type_;
    std::unique_ptr<StreamType> original_stream_type_;
    Graph* graph_;
    OutputRef output_;
    std::shared_ptr<MojoProducer> producer_;
    std::shared_ptr<MojoPullModeProducer> pull_mode_producer_;
    std::shared_ptr<NullSink> null_sink_;
  };

  // Handles the completion of demux initialization.
  void OnDemuxInitialized(Result result);

  // Runs a seek callback.
  static void RunSeekCallback(const SeekCallback& callback);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  Array<MediaTypeSetPtr> allowed_media_types_;
  Graph graph_;
  PartRef demux_part_;
  std::shared_ptr<Demux> demux_;
  Incident init_complete_;
  std::vector<std::unique_ptr<Stream>> streams_;
  MojoPublisher<GetStatusCallback> status_publisher_;
};

}  // namespace media
}  // namespace mojo

#endif  // MOJO_SERVICES_MEDIA_FACTORY_MEDIA_SOURCE_IMPL_H_
